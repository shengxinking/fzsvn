/**
 *	@file	mempool.h
 *
 *	@brief	mempool is a pool for malloc arbitrary memory size, the alloced
 *		memory not need release after use, it'll released when the pool
 *		destroyed.
 *
 *	@date	2009-03-16
 *
 *	@author	Forrest.zhang
 */

#ifndef FZ_MEMPOOL_H
#define FZ_MEMPOOL_H

#include "gcc_common.h"
#include "dbg_common.h"

#include <sys/types.h>

/* memory pool node */
typedef struct mempool_node {
	struct mempool_node	*next;	/* next memory node */
	char			data[0];/* the data */
} mempool_node_t;

/* memory pool */
typedef struct mempool {
	u_int32_t		max;	/* max alloced size */
	void			*last;	/* last position */
	void			*tail;	/* tail position */
	mempool_node_t		*nodes;	/* node list */
} mempool_t;

/**
 *	Alloc a memory node for memory pool @pool, new node can 
 *	contain @n bytes.
 *
 *	Return pointer if success, NULL on error.
 */
static inline void * 
_mp_alloc_node(mempool_t *pool, size_t n)
{
	void *ptr;
	size_t max, newsize;
	mempool_node_t *node;

	max = n > pool->max ? n : pool->max;
	newsize = align_num(max + sizeof(mempool_node_t));

	ptr = malloc(newsize);
	if (!ptr)
		ERR_RET(NULL, "malloc memory for node failed\n");

	node = ptr;
	node->next = pool->nodes;
	pool->nodes = node;
	
	ptr = align_ptr(ptr + sizeof(mempool_node_t));
	pool->last = ptr + n;
	pool->tail = (void *)node + newsize;
	pool->max = max;

	return ptr;
}


/**
 *	Malloc memory from memory pool @pool, required memory size is
 *	@size, if @zero is set, it'll cleanup the returned memory.
 *
 *	Return pointer if success, NULL on error.
 */
static inline void * 
_mp_malloc(mempool_t *pool, size_t size, int zero)
{
	void *ptr;

	if (!pool || size < 1)
		ERR_RET(NULL, "invalid argument\n");

	if (size < pool->max) {
		ptr = align_ptr(pool->last);
		if ((size_t)(pool->tail - ptr) > size) 
		{
			pool->last = ptr + size;
			if (zero) {
				memset(ptr, 0, size);
			}
			return ptr;
		}
	}

	return _mp_alloc_node(pool, size);
}

/**
 *	Alloc a new memory pool, the pool size is @size.
 *
 *	Return non-void pointer if success, NULL on error
 */
static inline mempool_t *
mempool_create(size_t size)
{
	size_t newsize;
	mempool_t *pool = NULL;
	
	if (size < 1)
		ERR_RET(NULL, "invalid argument\n");
	
	newsize = align_num(size + sizeof(mempool_t));

	pool = malloc(newsize);
	if (!pool) 
		ERR_RET(NULL, "malloc mempool error: %s\n", strerror(errno));

	pool->max = size;

	pool->last = (char *)pool + sizeof(mempool_t);
	pool->tail = (char *)pool + newsize;
	pool->nodes = NULL;
	
	return pool;
}

/**
 *	Free memory pool @pool when it's not used. All memory 
 *	in it will free.
 *
 *	No return value
 */
static inline void
mempool_destroy(mempool_t *pool)
{
	mempool_node_t *n, *p;

	if (unlikely(!pool)) {
		ERR("invalid argument\n");
		return;
	}
	
	/* free node */
	n = pool->nodes;
	while (n) {
		p = n->next;
		free(n);
		n = p;
	}

	/* free pool */
	free(pool);
}

/**
 *	Get a free memory from memory pool @pool, the returned
 *	memory size is @size. 
 *	
 *	Return non-void pointer if success, NULL on error.
 */
static inline void *
mempool_alloc(mempool_t *pool, size_t size)
{
	return _mp_malloc(pool, size, 0);
}

/**
 *	Get a free memory from mempool_t @pool, the returned
 *	memory size is @size and is zeroed.
 *	
 *	Return non-void pointer if success, NULL on error.
 */
static inline void *
mempool_calloc(mempool_t *pool, size_t size)
{
	return _mp_malloc(pool, size, 1);
}

/**
 *	Realloc memory for @ptr, the old content of @ptr is saved in
 *	new malloced memory.
 *
 *	Return non-void pointer if success, NULL on error.
 */
static inline void * 
mempool_realloc(mempool_t *pool, void *ptr, size_t oldsize, size_t newsize)
{
	void *newptr;
	size_t copysize;

	if (!pool || !ptr || oldsize < 1 || newsize < 1)
		ERR_RET(NULL, "invalid argument\n");

	copysize = oldsize < newsize ? oldsize : newsize;
	
	newptr = _mp_malloc(pool, newsize, 0);
	if (!newptr)
		return NULL;

	memcpy(newptr, ptr, copysize);
	if (newsize > copysize)
		memset(newptr + copysize, 0, newsize - copysize);

	return newptr;
}

/**
 *	Print the memory pool @pool in stdout.
 *
 *	No return value
 */
static inline void 
mempool_print(const mempool_t *pool)
{
	mempool_node_t *node;

	if (unlikely(!pool))
		return;

	printf("pool(%p): max %u, last(%p), tail(%p)\n", 
		pool, pool->max, pool->last, pool->tail);
	node = pool->nodes;
	while(node) {
		printf("\tnode(%p): next(%p)\n", node, node->next);
		node = node->next;
	}
}


#endif /* end of FZ_MEMPOOL_H */


