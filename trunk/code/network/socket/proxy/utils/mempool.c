/**
 *	@file	mempool.c
 *	@brief	the mempool implement, it provide free object for other function
 *		It's only used in fixed size object.
 *
 *	@author	FZ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "mempool.h"

/**
 *	Define some MACRO for printing error message
 */
#define _MP_DBG
#ifdef _MP_DBG
#define _MP_ERR(fmt, args...)	fprintf(stderr, "mempool:%s:%d: " fmt, \
						__FILE__, __LINE__, ##args)
#else
#define _MP_ERR(fmt, args...)
#endif

/*	The memory align macro */
#ifdef	__i386__
//#define	_MP_ALIGN(n)	(((n + 3) / 4) * 4)
#else
#define	_MP_ALIGN(n)	(((n + 7) / 8) * 8)
#endif

/* Get pointer to _mempool_node accoring @mempool_node_t->data */
#define _MP_OFFSET(p)	(void *)((char *)p - _MP_ALIGN(sizeof(_mp_node_t)))


struct _mp_node;


/**
 *	The memory cache, all objects stored in it.
 *
 */
typedef	struct _mp_cache {
	struct _mp_cache *next, **pprev;/* the list pointer */
	void		*pool;		/* the memory for object */
	u_int32_t	magic;		/* magic nunber, same as memory pool */
	u_int32_t	capacity;	/* total number of object in cache */
	u_int32_t	nused;		/* number of used object */
} _mp_cache_t;


/**
 *	Struct _mempool_node is the header of each memory 
 *	unit in memory pool. 
 */
typedef struct _mp_node {
	struct _mp_node	*next, **pprev;	/* list pointer in cache */
	_mp_cache_t	*cache;		/* the cache it belongs */
	mempool_t	*pool;		/* the pool it belongs */
	u_int32_t	magic;		/* magic number, same as memory pool */
	u_int32_t	is_used:1;	/* this node is used */
} _mp_node_t;


/**
 *	Lock the memory pool @mp, it's for thread-safe
 *
 *	No return.
 */
static void 
_mp_lock(mempool_t *mp)
{
	assert(mp);
	
	if (mp->need_lock) {
		pthread_mutex_lock(&mp->lock);
	}
}


/**
 *	Unlock the memory pool @mp, it's for thread-safe
 *
 *	No return.
 */
static void 
_mp_unlock(mempool_t *mp)
{
	assert(mp);

	if (mp->need_lock) {
		pthread_mutex_unlock(&mp->lock);
	}
}


/**
 *	Check system memory usage.
 *
 * 	Return 0 if can alloc memory, -1 if memory usage is too high
 */
static int 
_mp_check_memory(void)
{
	return 0;
}


/**
 *	Alloc a memory cache, it can hold MEMPOLL_INS_SIZE object.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_mp_alloc_cache(mempool_t *mp)
{
	_mp_cache_t *cache = NULL;
	_mp_node_t *node = NULL;
	int i;

	assert(mp);

	if (_mp_check_memory())
		return -1;

	cache = malloc(sizeof(_mp_cache_t));
	if (!cache) {
		_MP_ERR("alloc memory for cache failed\n");
		return -1;
	}

	cache->pool = malloc(mp->nodesize * mp->incsize);
	if (!cache->pool) {
		return -1;
	}

	/* put all memory unit into free_list */
	for (i = 0; i < mp->incsize; i++) {
		node = (_mp_node_t *)((char *)cache->pool + i * mp->nodesize);
		memset(node, 0, mp->nodesize);
		node->magic = mp->magic;
		node->cache = cache;
		node->pool = mp;
	
		/* add to @mp->node_list */
		node->pprev = (_mp_node_t **)&mp->node_list;
		node->next = mp->node_list;
		mp->node_list = node;
	}

	cache->magic = mp->magic;
	cache->capacity = mp->incsize;
	cache->nused = 0;
	mp->nalloced += mp->incsize;
	mp->nfreed += mp->incsize;

	/* add it to @mp->cache_list */
	cache->next = mp->cache_list;
	cache->pprev = (_mp_cache_t **)&mp->cache_list;
	mp->cache_list = cache;

	return 0;
}


/**
 *	Delete cache @cache from memory pool @mp's cache list.
 *	and remove all free nodes in @mp->node_list.
 *
 *	No return.
 */
static void 
_mp_free_cache(mempool_t *mp, _mp_cache_t *cache)
{
	_mp_node_t *node;
	int i;

	assert(mp);
	assert(cache);
	assert(cache->nused == 0);
	assert(cache->pprev);
	assert(cache->pool);
	assert(mp->magic == cache->magic);
	
	if (cache->next)
		cache->next->pprev = cache->pprev;
	*cache->pprev = cache->next;

	cache->pprev = NULL;
	cache->next = NULL;

	for (i = 0; i < mp->incsize; i++) {
		node = (_mp_node_t *)((char *)cache->pool + mp->nodesize);
		assert(node->is_used == 0);
		assert(node->pprev);

		/* delete it from @mp->node_list */
		if (node->next)
			node->next->pprev = node->pprev;
		*node->pprev = node->next;
	}

	free(cache->pool);
	free(cache);
}


/**
 *	Free cache list in memory pool @mp
 *
 *	No return.
 */
static void 
_mp_free_cache_list(mempool_t *mp)
{
	_mp_cache_t *cache, *next;

	assert(mp);

	cache = mp->cache_list;
	
	while (cache) {
		next = cache->next;
		if (cache->pool)
			free(cache->pool);
		free(cache);
		cache = next;
	}

	mp->cache_list = NULL;
}


/**
 *	print the cache information.
 *
 *	No return.
 */
static void 
_mp_print_cache(const _mp_cache_t *cache)
{
	assert(cache);

	printf("  cache(%p): magic %u, pool %p, capacity %u, nused %u, pprev %p, next %p\n",
	       cache, cache->magic, cache->pool, cache->capacity, 
	       cache->nused, cache->pprev, cache->next);
}


/**
 *	Put a node into memory pool @mp
 *
 *	No return.
 */
static void 
_mp_put_node(mempool_t *mp, _mp_node_t *node)
{
	_mp_cache_t *cache;
	
	assert(mp);
	assert(node);
	assert(node->cache);

	cache = node->cache;

	assert(mp->magic == node->magic);
	assert(mp->magic == cache->magic);
	assert(node->is_used == 1);
	
	/* add node to node_list */
	node->pprev = (_mp_node_t **)&mp->node_list;
	node->next = mp->node_list;
	mp->node_list = node;
	
	node->is_used = 0;
	mp->nfreed++;
	cache->nused--;

	assert(cache->nused >= 0);

	/**
	 * if all nodes in cache is freed and too many 
	 * free nodes, free this cache 
	 */
	if (cache->nused == 0 && mp->nfreed > 2 * mp->incsize)
		_mp_free_cache(mp, cache);

}


/**
 *	print the _mempool_node's to stdout
 *
 *	No return.
 */
static void 
_mp_print_node(const _mp_node_t *node)
{
	assert(node);
	
	printf("    node(%p): magic %u, is_used %x, cache %p, pool %p, pprev %p, next %p\n",
	       node, node->magic, node->is_used, node->cache, 
	       node->pool, node->pprev, node->next);
}


mempool_t * 
mempool_alloc(size_t objsize, int incsize, int locked)
{
	mempool_t *mp = NULL;

	if (objsize < 1)
		return NULL;

	if (incsize < 1)
		incsize = MEMPOOL_INC_SIZE;

	mp = malloc(sizeof(mempool_t));
	if (!mp) {
		_MP_ERR("malloc mempool error: %s\n", strerror(errno));
		return NULL;
	}
	memset(mp, 0, sizeof(mempool_t));

	mp->magic = time(NULL);
	mp->objsize = _MP_ALIGN(objsize);
	mp->incsize = incsize;
	mp->nodesize = mp->objsize + _MP_ALIGN(sizeof(_mp_node_t));

	/* using create time as magic number, so it's unique */

	if (_mp_alloc_cache(mp)) {
		free(mp);
		return NULL;
	}

	/* initiate pthread lock for thread-safe */
	if (locked) {
		mp->need_lock = 1;
		pthread_mutex_init(&mp->lock, NULL);
	}

	return mp;
}


void 
mempool_free(mempool_t *mp)
{
	if (!mp)
		return;

	_mp_free_cache_list(mp);

	free(mp);
}


void * 
mempool_get(mempool_t *mp)
{
	_mp_node_t *node = NULL;

	if (!mp)
		return NULL;

	_mp_lock(mp);

	/* no freed memory unit */
	if (mp->nfreed == 0 && _mp_alloc_cache(mp)) {
		_mp_unlock(mp);
		return NULL;
	}

	/* get node from @mp->node_list */
	node = mp->node_list;
	if (node->next)
		node->next->pprev = (_mp_node_t **)&mp->node_list;
	mp->node_list = node->next;

	node->is_used = 1;
	
	assert(node->cache);
	node->cache->nused++;
	mp->nfreed--;

	_mp_unlock(mp);

	return ((char *)node + _MP_ALIGN(sizeof(_mp_node_t)));
}


int 
mempool_put1(mempool_t *mp, void *ptr)
{
	_mp_node_t *node = NULL;

	if (!mp || !ptr)
		return -1;
	
	node = _MP_OFFSET(ptr);

	_mp_lock(mp);
	
	_mp_put_node(mp, node);
	
	_mp_unlock(mp);
	
	return 0;
}


int 
mempool_put(void *ptr)
{
	_mp_node_t *node = NULL;
	mempool_t *mp = NULL;

	if (!ptr)
		return -1;
	
	node = _MP_OFFSET(ptr);

	assert(node->pool);
	mp = node->pool;

	_mp_lock(mp);
	
	_mp_put_node(mp, node);
	
	_mp_unlock(mp);
	
	return 0;
}



void 
mempool_print(mempool_t *mp)
{
	_mp_cache_t *cache;
	_mp_node_t *node;

	if (!mp)
		return;

	_mp_lock(mp);

	printf("pool(%p): magic %u, nalloced %u, nfreed: %u\n",
	       mp, mp->magic, mp->nalloced, mp->nfreed);

	cache = mp->cache_list;
	while (cache) {
		_mp_print_cache(cache);
		cache = cache->next;
	}

	node = mp->node_list;
	while (node) {
		_mp_print_node(node);
		node = node->next;
	}

	_mp_unlock(mp);
}

