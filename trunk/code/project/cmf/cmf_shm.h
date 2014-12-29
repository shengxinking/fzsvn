/**
 *	@file	cmf_memory.h
 *
 *	@brief	the CMF memory APIs.
 */

#ifndef	CMF_MEMORY_H
#define	CMF_MEMORY_H

#include <sys/types.h>

#include "cmf_api.h"

#define	CMF_SHM_PATH	"/dev/shm/cmf/"


typedef	int	cmf_shm_ptr_t;

/* the control file header */
typedef struct cmf_shm_ctl {
	u_int32_t	magic;		/* magic number */
	u_int32_t	size;		/* the total file length */
	u_int8_t	nodemap[0];	/* the node map */
} cmf_shm_ctl_t;

/* the object header */
typedef struct cmf_shm_node {
	u_int32_t	magic;		/* the magic number */
	int		is_used;	/* is used */
	u_int32_t	len;		/* the object length */	
	char		data[0];	/* the object array */
} cmf_shm_node_t;

/* the object file header */
typedef struct cmf_shm_obj {
	u_int32_t	magic;		/* the magic number */
	u_int32_t	size;		/* the total file length */
	cmf_shm_node_t	node[0];	/* the node array */
} cmf_shm_obj_t;

/* the cmf cache structure */
typedef	struct cmf_shm_cache {
	u_int32_t	objsize;	/* the object size */
	u_int32_t	incsize;	/* the increment size */
	u_int32_t	max;		/* max objects */
	int		nfree;		/* number of freed object */
	cmf_shm_ctl_t	*ctlfile;	/* the control file */
	cmf_shm_obj_t	*objfile;	/* the object file */
} cmf_shm_cache_t;

/**
 *	Create a new cmf_cache object, which the object size 
 *	is @objsize, the object number is @objnum. the increment
 *	size is @incsize.
 *
 *	Return the ptr if success, NULL on error.
 */
extern cmf_shm_cache_t *  
cmf_shm_create(const char *name, int objsize, int objnum, int incsize);

/**
 *	Init a cmf_shm_cache_t according file name @name.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_shm_init(cmf_shm_cache_t *cache, const char *name);

/**
 *	Destroy a cmf_shm_cache_t object @cache, remove control
 *	file and data file.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_shm_destroy(cmf_shm_cache_t *cache);

/**
 *	Alloc a object from cmf_shm_cache_t @cache.
 *
 *	Return >0 if success, -1 on error.
 */
extern cmf_shm_ptr_t  
cmf_shm_alloc(cmf_shm_cache_t *cache);

/**
 *	Free a object to cmf_shm_cache_t @cache.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_shm_free(cmf_shm_cache_t *cache, cmf_shm_ptr_t ptr);

/**
 *	Convert a cmf_ptr_t to void pointer.
 *
 *	Return pointer if success, NULL on error.
 */
extern void *  
cmf_ptr2void(cmf_shm_cache_t *cache, cmf_shm_ptr_t ptr);

/**
 *	Convert a void pointer to cmf_ptr_t object.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern cmf_shm_ptr_t  
cmf_void2ptr(cmf_shm_cache_t *cache, void *ptr);

#endif	/* end of CMF_MEMORY_H */


