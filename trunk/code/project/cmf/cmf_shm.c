/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cmf_shm.h"

#define	CMF_SHM_ALIGN(n)	(((n + 7) / 8) * 8)
#define	CMF_SHM_OFFSET(p)	\
	(void *))((char *)p - CMF_SHM_ALIGN(sizeof(cmf_shm_node_t)

#define	CMF_SHM_DEBUG

#ifdef	CMF_SHM_DEBUG
#define	CMF_SHM_DBG(fmt, a...)	printf("[cmf-shm]: "fmt, ##a)
#else
#define	CMF_SHM_DBG(fmt, a...)
#endif

#define	CMF_SHM_ERR(fmt, a...)	printf("[cmf-shm](%s-%d): "fmt, \
				       __FILE__, __LINE__, ##a)


static int 
_cmf_shm_alloc_ctl(cmf_shm_cache_t *cache, const char *fname, 
		   int objnum, int is_inc)
{
	size_t msize = 0;
	size_t fsize = 0;
	size_t osize = 0;
	int mflag;
	struct stat st;
	int fd;

	/* check exist control file */
	if (!is_inc && access(fname) == F_OK) {
		CMF_SHM_ERR("the control file %s exist\n", fname);
		return -1;
	}

	/* open control file, if not exist, create it */
	fd = open(fname, O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		CMF_SHM_ERR("open control file %s failed\n", fname);
		return -1;
	}

	/* calc the mmap size */
	if (is_inc) {
		if (fstat(fd, &st)) {
			CMF_SHM_ERR("fstat %s failed\n", fname);
			close(fd);
			unlink(fname);
			return -1;
		}
		fsize = st.st_size;
		if (fsize != cache->ctlfile->size) {
			CMF_SHM_ERR("%s fsize is incorrect\n", fname);
			unlink(fname);
			close(fd);
			return -1;
		}
		msize = fsize + objnum * sizeof(u_int8_t);
	}
	else {
		fsize = 0;
		msize = sizeof(*cache) + objnum * sizeof(u_int8_t);
	}

	/* mmap the file */
	mflag = PROT_WRITE|PROT_READ|MAP_PRIVATE;
	cache->ctlfile = mmap(NULL, msize, mflag, 0, fd, 0);
	if (!cache->ctlfile) {
		CMF_SHM_ERR("mmap control file %s failed\n", fname);
		close(fd);
		unlink(fname);
		return -1;
	}

	/* memset the new object */
	if (is_inc)
		osize = objnum * sizeof(u_int8_t);
	else 
		osize = sizeof(cmf_shm_ctl_t) + objnum * sizeof(u_int8_t);
	memset(cache->ctlfile + fsize, 0, osize);
		
	return 0;
}

static int 
_cmf_shm_create_obj(cmf_shm_cache_t *cache, const char *fname,
		    int objsize, int objnum, u_int32_t magic)
{
	size_t msize = 0;
	size_t fsize = 0;
	size_t osize = 0;
	int mflag;
	struct stat st;
	cmf_shm_node_t *node;
	int fd;

	/* check exist control file */
	if (!is_inc && access(fname) == F_OK) {
		CMF_SHM_ERR("the object file %s exist\n", fname);
		return -1;
	}

	/* open object file, if not exist, create it */
	fd = open(fname, O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		CMF_SHM_ERR("open object file %s failed\n", fname);
		return -1;
	}

	/* calc the mmap size */
	if (is_inc) {
		if (fstat(fd, &st)) {
			CMF_SHM_ERR("fstat %s failed\n", fname);
			close(fd);
			unlink(fname);
			return -1;
		}
		fsize = st.st_size;
		if (fsize != cache->objfile->size) {
			CMF_SHM_ERR("%s fsize is incorrect\n", fname);
			unlink(fname);
			close(fd);
			return -1;
		}
		msize = fsize + objnum * objsize;
	}
	else {
		fsize = 0;
		msize = sizeof(*cache) + objnum * sizeof(u_int8_t);
	}

	/* mmap the file */
	mflag = PROT_WRITE|PROT_READ|MAP_PRIVATE;
	cache->objfile = mmap(NULL, msize, mflag, 0, fd, 0);
	if (!cache->objfile) {
		CMF_SHM_ERR("mmap object file %s failed\n", fname);
		close(fd);
		unlink(fname);
		return -1;
	}

	/* memset the new object */
	if (is_inc) {
		osize = objnum * sizeof(u_int8_t);
		offset = fsize;
	}
	else { 
		osize = sizeof(cmf_shm_ctl_t) + objnum * sizeof(u_int8_t);
		offset = sizeof(cmf_shm_ctl_t)
	}
	memset(cache->ctlfile + fsize, 0, osize);
	
	/* init the node array */
	for (i = 0; i < objnum; i++) {
		offset = i * sizeof(cmf_shm_node_t);
		node = cache->objfile + offset;
		node->magic = magic;
		node->len = objsize;
	}
	
	return 0;
}

cmf_shm_cache_t *  
cmf_shm_create(const char *name, int objsize, int objnum, int incsize)
{
	cmf_shm_cache_t *cache;
	char ctl_fname[CMF_NAMELEN];
	char obj_fname[CMF_NAMELEN];
	u_int32_t magic;
	size_t len;
	int fd;

	if (!name || objsize < 1 || objnum < 1 || incsize < 1) {
		CMF_SHM_ERR("invalid argument\n");
		return NULL;
	}

	magic = time(NULL);

	/* malloc cache */
	cache = malloc(sizeof(cmf_shm_cache_t));
	if (!cache) {
		CMF_SHM_ERR("malloc memory for cache failed\n");
		return NULL;
	}
	memset(cache, 0, sizeof(cmf_shm_cache_t));
	
	/* create control file */
	

	/* create object file */
	

	/* set cache variable */
	cache->max = objnum;
	cache->objszie = objsize;
	cache->

	return cache;
}

int 
cmf_shm_init(cmf_shm_cache_t *cache, const char *name)
{
	return 0;
}

int 
cmf_shm_destroy(cmf_shm_cache_t *cache)
{
	return 0;
}

cmf_shm_ptr_t  
cmf_shm_alloc(cmf_shm_cache_t *cache)
{
	cmf_shm_node_t *node;

	return -1;
}

int 
cmf_shm_free(cmf_shm_cache_t *cache, cmf_shm_ptr_t ptr)
{
	cmf_shm_node_t *node;

	return 0;
}

void * 
cmf_shm_ptr2void(cmf_shm_cache_t *cache, cmf_shm_ptr_t ptr)
{
	return NULL;
}

cmf_shm_ptr_t  
cmf_shm_void2ptr(cmf_shm_cache_t *cache, void *ptr)
{
	return 0;
}



