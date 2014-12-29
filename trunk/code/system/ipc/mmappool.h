/**
 *	@file	mmappool.h
 *
 *	@brief	Object pool using mmap file implement.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_MMAPPOOL_H
#define FZ_MMAPPOOL_H

#define	MPOOL_MAX	65536
#define	MPOOL_NULL	0xffffffff

typedef	u_int32_t	mmappool_id_t;

typedef struct mmappool {
	u_int32_t	magic;		/* magic number */
	u_int32_t	nalloced;	/* max object number */
	u_int32_t	nfreed;		/* used object number */
	u_int32_t	objsize;	/* object size */
	u_int32_t	nodesize;	/* node size */
	mmappool_id_t	freepos;	/* free node list */
	pthread_mutex_t	lock;		/* lock for thread/process safe */
} mmappool_t;

/**
 *	Create a mmapped pool which file name is @file.
 *	The increment size is @incsize.
 *
 * 	Return non-NULL pointer if success, NULL on error.
 */
extern mmappool_t * 
mmappool_create(const char *file, size_t incsize.);

/**
 *	Open a mmapped pool which file name is @file.
 *
 * 	Return non-NULL pointer if success, NULL on error.
 */
extern mmappool_t *
mmappool_open(const char *file);

/**
 *	Close a mmapped pool @pool.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
mmappool_close(mmappool_t *pool);

/**
 *	Alloc a free object from mmapped pool @pool. 
 *	If not free space, it'll enlarge the pool by 
 *	incsize number object.
 *
 */
extern mmappool_id_t 
mmappool_alloc(mmappool_t **pool);

extern int 
mmappool_free(mmappool_t *pool, mmappool_id_t id);

extern int 
mmappool_copyin(mmappool_t *pool, mmappool_id_t id, void *ptr, size_t siz);

extern int 
mmappool_copyout(mmappool_t *pool, mmappool_id_t id, const void *ptr, size_t siz);

extern void 
mmappool_print(const mmappool_t *pool);


#endif /* end of FZ_MMAP_FILE_H  */

