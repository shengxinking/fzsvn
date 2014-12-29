/**
 *	@file	ojbpool.h
 *
 *	@brief	objpool is a big alloced memory for fixed size 
 *		object, the other function can use it to get/put 
 *		objects fast than malloc/free.
 *		it can only used in fixed size object.
 *
 *	@date	2009-03-16
 *
 *	@author	Forrest.zhang
 */

#ifndef FZ_OBJPOOL_H
#define FZ_OBJPOOL_H

#include <pthread.h>
#include <sys/types.h>

#define	OBJPOOL_INC_SIZE	1000	/* alloc obj number one time*/

/**
 *	Struct objpool is a object pool, you can get free object 
 *	unit from it(using @pktpool_get), or put a unused object 
 *	to it(using @objpool_put).
 *
 *	All object need have same size @objsize, so don't exceed 
 *	it's size when using it.
 */
typedef struct objpool {
	u_int32_t	magic;		/* magic number for verify */
	u_int16_t	need_lock:1;	/* the objpool need locked */
	u_int16_t	objsize;	/* the object memory size */
	u_int16_t	incsize;	/* inceament size */
	u_int16_t	nodesize;	/* the node memory size */
	u_int32_t	nalloced;	/* number of alloced objects*/
	u_int32_t	nfreed;		/* number of freed objects */
	void		*node_list;	/* the freed node list */
	void		*cache_list;	/* the cache list */
	pthread_mutex_t	lock;		/* lock for thread safe */
} objpool_t;

/**
 *	Alloc a new memory pool, the each unit in memory pool size
 *	is @objsize, it provide @objnum units. If @locked is not zero,
 *	this memory pool is thread safed.
 *
 *	Return non-void pointer if success, NULL means error
 */
extern objpool_t*
objpool_alloc(size_t objsize, int incsize, int locked);

/**
 *	Free memory pool @pool when it's not used.
 *
 *	No return value
 */
extern void 
objpool_free(objpool_t *pool);

/**
 *	Get a free memory unit from memory pool @pool, the returned
 *	memory area size is @objsize when @pool is create. 
 *	
 *	Return non-void pointer if success, NULL means error.
 */
extern void*
objpool_get(objpool_t *pool);

/**
 *	Put a memory unit into memory pool @pool, so it can be used
 *	later. The returned memory unit need be one which is get 
 *	from the @pool early.
 *
 *	Return 0 if success, return -1 if error. 
 */
extern int 
objpool_put1(objpool_t *pool, void *data);

/**
 *	Put a memory unit into memory pool, the memory pool is hidden
 *      in @data, see implement.
 *
 *	Return 0 if success, -1 on error.
 */
extern int
objpool_put(void *data);

/**
 *	Print the memory pool @pool in stdout.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
objpool_print(objpool_t *pool);


#endif /* end of FZ_OBJPOOL_H */


