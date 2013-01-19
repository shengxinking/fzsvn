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

/**
 *	mempool's flags, and some MACRO to set/test/clear
 */
#define MEMPOOL_BIT_LOCK	0
#define MEMPOOL_LOCK	(1 << (MEMPOOL_BIT_LOCK)) /* mempool need lock for thread-safe */

#define MEMPOOL_IS_LOCK(flags)	(flags & MEMPOOL_LOCK)
#define MEMPOOL_SET_LOCK(flags)	(flags |= MEMPOOL_LOCK)
#define MEMPOOL_CLR_LOCK(flags)	(flags &= ~MEMPOOL_LOCK)

/**
 *	Struct mempool is a memory pool, you can get free memory unit from 
 *	it(using @mempool_get), or put a unused memory to it(using
 *	@mempool_put).
 *
 *	All memory unit have same size @objsize, so don't exceed it's size \
 *	when using it.
 */
typedef struct mempool {
	u_int32_t	magic;	/* magic number for verify */
	u_int32_t	flags;	/* flags */
	u_int32_t	size;	/* how many object alloced */
	u_int32_t	objsize;/* object size */
	void		*pool;	/* the alloced big memory */
	void		*flist;	/* the freed objects list */
	u_int32_t	nfreed;	/* number of freed objects */
	pthread_mutex_t	lock;	/* lock for thread safe */
} mempool_t;

/**
 *	Alloc a new memory pool, the each unit in memory pool size
 *	is @objsize, it provide @objnum units. If @locked is not zero,
 *	this memory pool is thread safed.
 *
 *	Return non-void pointer if success, NULL means error
 */
extern mempool_t*
mempool_alloc(size_t objsize, size_t objnum, int locked);

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
mempool_put(mempool_t *cache, void *data);

/**
 *	Put a memory unit into memory pool, the memory pool is hidden
 *      in @data, see implement.
 *
 *	Return 0 if success, -1 on error.
 */
extern int
mempool_put1(void *data);


/**
 *	Print the memory pool @pool in stdout.
 *
 *	No return value
 */
extern void 
mempool_print(mempool_t *pool);


#endif /* end of FZ_MEMPOOL_H */


