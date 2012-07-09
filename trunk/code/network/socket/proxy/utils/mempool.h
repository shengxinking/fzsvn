/**
 *	@file	mempool.h
 *
 *	@brief	mempool is a big alloced memory for fixed size object, the other
 *		function can use it to get/put memory fast than malloc/free.
 *		it can only used in fixed size object.
 *
 *	@date	2009-03-16
 *
 *	@author	Forrest.zhang
 */

#ifndef FZ_MEMPOOL_H
#define FZ_MEMPOOL_H

#include <pthread.h>
#include <sys/types.h>

#define	MEMPOOL_INC_SIZE		1000	/* alloc 1000 objects one time */

/**
 *	Struct mempool is a memory pool, you can get free memory unit from 
 *	it(using @mempool_get), or put a unused memory to it(using
 *	@mempool_put).
 *
 *	All memory unit have same size @objsize, so don't exceed it's size \
 *	when using it.
 */
typedef struct mempool {
	u_int32_t	magic;		/* magic number for verify */
	u_int16_t	objsize;	/* the object memory size */
	u_int16_t	incsize;
	u_int16_t	nodesize;	/* the node memory size */
	u_int16_t	need_lock:1;	/* the mempool need locked */
	void		*cache_list;	/* the cache list */
	void		*node_list;	/* the freed node list */
	u_int32_t	nalloced;	/* number of alloced objects */
	u_int32_t	nfreed;		/* number of freed objects */
	pthread_mutex_t	lock;		/* lock for thread safe */
} mempool_t;

/**
 *	Alloc a new memory pool, the each unit in memory pool size
 *	is @objsize, it provide @objnum units. If @locked is not zero,
 *	this memory pool is thread safed.
 *
 *	Return non-void pointer if success, NULL means error
 */
extern mempool_t*
mempool_alloc(size_t objsize, int incsize, int locked);

/**
 *	Free memory pool @pool when it's not used.
 *
 *	No return value
 */
extern void 
mempool_free(mempool_t *pool);

/**
 *	Get a free memory unit from memory pool @pool, the returned
 *	memory area size is @objsize when @pool is create. 
 *	
 *	Return non-void pointer if success, NULL means error.
 */
extern void*
mempool_get(mempool_t *pool);

/**
 *	Put a memory unit into memory pool @pool, so it can be used
 *	later. The returned memory unit need be one which is get 
 *	from the @pool early.
 *
 *	Return 0 if success, return -1 if error. 
 */
extern int 
mempool_put1(mempool_t *pool, void *data);

/**
 *	Put a memory unit into memory pool, the memory pool is hidden
 *      in @data, see implement.
 *
 *	Return 0 if success, -1 on error.
 */
extern int
mempool_put(void *data);


/**
 *	Print the memory pool @pool in stdout.
 *
 *	No return value
 */
extern void 
mempool_print(mempool_t *pool);


#endif /* end of FZ_MEMPOOL_H */


