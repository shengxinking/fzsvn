/**
 *	@file	cmf_memory.h
 *
 *	@brief	the CMF memory APIs.
 */

#ifndef	CMF_MEMORY_H
#define	CMF_MEMORY_H

#define	CMF_NAMELEN	64
#define	CMF_PATH	"/var/run/cmf/"

typedef	unsigned long	cmf_ptr_t;

typedef struct cmf_page {
	char		file[CMF_NAMELEN];
	cmf_ptr_t	cache;
} cmf_page_t;

typedef	struct cmf_cache {
	char		file[CMF_NAMELEN];
	u_int32_t	len;
} cmf_cache_t;

typedef struct cmf_slab {
	cmf_ptr_t	page;
	int		freeblock;
	u_int8_t	nodemap[0];
} cmf_slab_t;


extern int 
cmf_init_memory(void);

extern int 
cmf_free_memory(void;

extern cmf_ptr_t 
cmf_alloc_memory(int size);

extern void 
cmf_free_memory(cmf_ptr_t ptr);

extern void *
cmf_get_ptr(cmf_ptr_t ptr);


#endif	/* end of CMF_MEMORY_H */


