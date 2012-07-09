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
#define _MEMPOOL_DBG
#ifdef _MEMPOOL_DBG
#define _MEMPOOL_ERR(fmt, args...)	fprintf(stderr, "mempool:%s:%d: " fmt, \
						__FILE__, __LINE__, ##args)
#else
#define _MEMPOOL_ERR(fmt, args...)
#endif

/**
 *	_mempool_node's flags, and some macro to set/clear/get
 */
#define _MEMPOOL_BIT_USED	0
#define _MEMPOOL_USED		(1 << (_MEMPOOL_BIT_USED))

#define _MEMPOOL_IS_USED(flags)		(flags & _MEMPOOL_USED)
#define _MEMPOOL_SET_USED(flags)	(flags |= _MEMPOOL_USED)
#define _MEMPOOL_CLR_USED(flags)	(flags &= ~_MEMPOOL_USED)

/**
 *	Struct _mempool_node is the header of each memory 
 *	unit in memory pool. 
 */
typedef struct _mempool_node {
	u_int32_t		magic;
	u_int32_t		flags;
	void			*pool;
	struct _mempool_node	*next;
	u_int32_t		size;
	char			data[0];
} _mempool_node_t;


/* Each memory unit is align to 4, it's common in 32-bit Machine */
#define _MEMPOOL_ALIGN(n)	(((n + 3) / 4) * 4)

/* Get pointer to _mempool_node accoring @mempool_node_t->data */
#define _MEMPOOL_OFFSET(p)	(void *)((char *)p - sizeof(_mempool_node_t))


/**
 *	Lock the memory pool @pool, it's for thread-safe
 *
 *	No return.
 */
static void 
_mempool_lock(mempool_t *pool)
{
	assert(pool);
	
	if (MEMPOOL_IS_LOCK(pool->flags)) {
		pthread_mutex_lock(&pool->lock);
	}
}

/**
 *	Unlock the memory pool @pool, it's for thread-safe
 *
 *	No return.
 */
static void 
_mempool_unlock(mempool_t *pool)
{
	assert(pool);

	if (MEMPOOL_IS_LOCK(pool->flags)) {
		pthread_mutex_unlock(&pool->lock);
	}
}

/**
 *	Alloc a memory pool, memory unit size is @objsize, there have
 *	@objnum memory unit in this pool. If locked is not zero, it's
 *	thread-safe for using.
 *
 *	Return alloced mempool if success, NULL if error.
 */
mempool_t * 
mempool_alloc(size_t objsize, size_t objnum, int locked)
{
	mempool_t *pool = NULL;
	_mempool_node_t *node = NULL;
	int i = 0;
	int size = 0;
	u_int32_t magic;

	if (objsize < 1 || objnum < 1)
		return NULL;

	pool = malloc(sizeof(mempool_t));
	if (!pool) {
		_MEMPOOL_ERR("malloc mempool error: %s\n", strerror(errno));
		return NULL;
	}

	size = _MEMPOOL_ALIGN(objsize) + sizeof(_mempool_node_t);
	pool->pool = malloc(size * objnum);
	if (!pool->pool) {
		free(pool);
		return NULL;
	}

	/* using create time as magic number, so it's unique */
	magic = time(NULL);

	/* put all memory unit into list @pool->flist */
	pool->flist = pool->pool;
	for (i = 0; i < objnum; i++) {
		node = (_mempool_node_t *)((char *)pool->pool + i * size);
		memset(node, 0, size);
		node->size = size;
		node->magic = magic;
		node->pool = pool;
		if (i < objnum - 1)
			node->next = (_mempool_node_t *)
				((char *)pool->pool + (i + 1) * size);
		else 
			node->next = NULL;
	}

	pool->size = objnum;
	pool->objsize = objsize;
	pool->nfreed = objnum;
	pool->magic = magic;
	pool->flags = 0;

	/* initiate pthread lock for thread-safe */
	if (locked) {
		MEMPOOL_SET_LOCK(pool->flags);
		pthread_mutex_init(&pool->lock, NULL);
	}

	return pool;
}

/**
 *	Free memory pool @pool which is alloced using @mempool_alloc
 *
 *	No return.
 */
void 
mempool_free(mempool_t *pool)
{
	if (!pool)
		return;

	if (pool->nfreed < pool->size) {
		_MEMPOOL_ERR("mempool size(%u), used(%u), freed(%u)\n", 
			     pool->size, (pool->size - pool->nfreed), 
			     pool->nfreed);
	}
	
	if (pool->pool)
		free(pool->pool);

	free(pool);
}

/**
 *	Get a freed memory unit from memory pool @pool. If there is no 
 *	freed memory unit, return NULL, else return a freed memory unit.
 *
 *	Return not NULL if success, or NULL on failed.
 */
void * 
mempool_get(mempool_t *pool)
{
	_mempool_node_t *node = NULL;
	_mempool_node_t *freed = NULL;

	if (!pool)
		return NULL;

	_mempool_lock(pool);

	/* no freed memory unit */
	if (pool->nfreed < 1) {
		_mempool_unlock(pool);
		return NULL;
	}

	freed = pool->flist;
	node = freed;
	freed = node->next;

	node->flags = 0;
	_MEMPOOL_SET_USED(node->flags);
	node->next = NULL;

	pool->nfreed --;
	pool->flist = freed;

	_mempool_unlock(pool);

	return node->data;
}

/**
 *	Put a memory unit @ptr into memory pool @pool. It'll check
 *	the memory unit to see it's origin from this pool.
 *
 *	Return 0 if success, -1 means memory unit is invalid.
 */
int mempool_put(mempool_t *pool, void *ptr)
{
	_mempool_node_t *node = NULL;
	int ret = -1;

	if (!pool || !ptr)
		return -1;
	
	node = _MEMPOOL_OFFSET(ptr);

	_mempool_lock(pool);
	
	/* check magic number and flags */
	if (node->magic != pool->magic) {
		_MEMPOOL_ERR("magic number is invalid\n");
		goto OUT;
	}

	if (!_MEMPOOL_IS_USED(node->flags)) {
		_MEMPOOL_ERR("put a freed memory unit\n");
		goto OUT;
	}

	if (node->pool != pool) {
		_MEMPOOL_ERR("put memory to different pool\n");
		goto OUT;
	}

	node->next = pool->flist;
	pool->flist = node;
	pool->nfreed++;

	_MEMPOOL_CLR_USED(node->flags);

	ret = 0;

OUT:

	_mempool_unlock(pool);
	
	return ret;
}

/**
 *	Put a memory unit into memory pool, the memory pool is hidden
 *      in @data, see implement.
 *
 *	Return 0 if success, -1 on error.
 */
int
mempool_put1(void *ptr)
{
	_mempool_node_t *node = NULL;
	mempool_t *pool = NULL;
	int ret = -1;

	if (!ptr)
		return -1;
	
	node = _MEMPOOL_OFFSET(ptr);
	pool = node->pool;

	if (!pool)
		return -1;

	_mempool_lock(pool);
	
	/* check magic number and flags */
	if (node->magic != pool->magic) {
		_MEMPOOL_ERR("magic number is invalid\n");
		goto OUT;
	}

	if (!_MEMPOOL_IS_USED(node->flags)) {
		_MEMPOOL_ERR("put a freed memory unit\n");
		goto OUT;
	}

	if (node->pool != pool) {
		_MEMPOOL_ERR("put memory to different pool\n");
		goto OUT;
	}

	node->next = pool->flist;
	pool->flist = node;
	pool->nfreed++;

	_MEMPOOL_CLR_USED(node->flags);

	ret = 0;

OUT:

	_mempool_unlock(pool);
	
	return ret;

}

/**
 *	print the _mempool_node's to stdout
 *
 *	No return.
 */
static void 
_mempool_node_print(const _mempool_node_t *node)
{
	assert(node);
	
	printf("\tnode(%p): magic %u, flags %x, pool %p next %p, size %u\n",
	       node, node->magic, node->flags, node->pool, 
	       node->next,node->size);
}


/**
 *	Print memory pool @pool to stdout
 *
 *	No return.
 */
void mempool_print(mempool_t *pool)
{
	_mempool_node_t *node;

	if (!pool)
		return;

	_mempool_lock(pool);

	printf("pool(%p): magic %u, size %u, pool %p, freed: %u\n",
	       pool, pool->magic, pool->size, pool->pool, pool->nfreed);

	node = pool->flist;
	while (node) {
		_mempool_node_print(node);
		node = node->next;
	}

	_mempool_unlock(pool);
}

