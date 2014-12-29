/**
 *	@file	mmappool.c
 *
 *	@brief	mmap pool implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2014-12-07
 */

#include "gcc_common.h"
#include "dbg_common.h"

#include "mmappool.h"

#define	_MP_GET_NODE(p, id, node)				\
({								\
	node = (void *)p + align_num(sizeof(*p));		\
 	node = (void *)node + (p->nodesize + p->objsize) * id;	\
})

typedef struct mmappool_node {
	u_int32_t	magic;		/* magic number */
	int		is_used;	/* is used or not */
	mmappool_id_t	next;		/* the next node id */
} mmappool_node_t;

static int 
_mp_lock(mmappool_t *pool)
{
	if (pthread_mutex_lock(&pool->lock))
		ERR_RET(-1, "pthread_mutex_lock failed\n");

	return 0;
}

static int 
_mp_unlock(mmappool_t *pool)
{
	if (pthread_mutex_unlock(&pool->lock))
		ERR_RET(-1, "pthread_mutex_unlock failed\n");

	return 0;
}

mmappool_t *
mmappool_alloc(const char *file, size_t objsize, size_t size)
{
	int i;
	size_t n;
	size_t nsize;
	size_t psize;
	mmappool_t *pool;
	mmappool_node_t *node;
	pthread_mutexattr_t attr;

	if (unlikely(!file || objsize < 1 || inc < 1))
		ERR_RET(NULL, "invalid argument\n");

	if (access(file, F_OK) == 0) 
		ERR_RET(NULL, "the file is exist\n");

	/* open the file */
	fd = open(file, O_RDWR | O_CREAT, 0644);
	if (fd < 0) 
		ERR_RET(NULL, "create file %s failed: %s\n", file, ERRSTR);

	objsize = align_num(objsize);
	nsize = align_num(sizeof(mmapnode_t));
	psize = align_num(sizeof(mmappool_t));

	n = psize + (nsize + objsize) * incsize;
	pool = mmap(NULL, n, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0L);
	close(fd);
	if (!pool) 
		ERR_RET(NULL, "mmap %s failed: %s\n", file, ERRSTR);

	memset(pool, 0, psize);
	pool->magic = time(NULL);
	pool->max = size;
	pool->nfreed = size;
	pool->incsize = size;
	pool->objsize = objsize;
	pool->nodesize = nsize;

	/* init free node list */
	for (i = 0; i < size; i++) {
		_MP_GET_NODE(pool, i, node);
		node->magic = pool->magic;
		if (i == size - 1)
			node->next = MMAPPOOL_NULL;
		else
			node->next = (i + i);
	}

	/* set lock process shared */
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&lock, &attr);

	return pool;
}

mmappool_t *
mmappool_open(const char *file)
{
	size_t n;
	size_t nsize;
	size_t psize;
	mmappool_t *pool;

	if (unlikely(!file || objsize < 1 || max < 1 || inc < 1))
		ERR_RET(NULL, "invalid argument\n");

	if (access(file, F_OK) == 0) 
		ERR_RET(NULL, "the file is exist\n");

	fd = open(file, O_RDWR);
	if (fd < 0) 
		ERR_RET(NULL, "open file %s failed: %s\n", file, ERRSTR);

	/* only mapped header to get real size */
	psize = align_num(sizeof(*pool));
	pool = mmap(NULL, psize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0L);
	if (!pool) {
		close(fd);
		ERR_RET(NULL, "mmap %s failed: %s\n", file, ERRSTR);
	}
	
	/* real size of mmaped */
	nsize = align_num(sizeof(mmappool_node_t));
	n = psize + (nsize + pool->objsize) * pool->max;
	
	/* unmapped it before remmap to real size. */
	if (munmap(pool, psize)) {
		close(fd);
		ERR_RET(NULL, "munmmap %s failed: %s\n", file, ERRSTR);
	}
	
	pool = mmap(NULL, n, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0L);
	close(fd);
	if (!pool) 
		ERR_RET(NULL, "mapp %s failed: %s\n", file, ERRSTR);

	return pool;
}

int 
mmappool_close(mmappool_t *pool)
{
	size_t n;
	size_t nsize;
	size_t psize;

	if (unlikely(!pool))
		ERR_RET(-1, "invalid argument\n");
	
	psize = align_num(sizeof(*pool));
	nsize = align_num(sizeof(mmappool_node_t));
	n = psize + (nsize + pool->objsize) * pool->max;
	if (munmap(pool, n))
		ERR_RET(-1, "munmap failed\n");

	return 0;
}

mmappool_id_t
mmappool_alloc(mmappool_t **pool)
{
	mmappool_id_t id;
	mmappool_node_t *node;

	if (unlikely(!pool || !*pool))
		ERR_RET(MMAP_FILE_NULL, "invalid argument\n");

	_mp_lock(*pool);

	if (*pool->nfreed == 0) {
		*pool = _mp_incsize(*pool);
		if (!*pool) {
			_mp_unlock(*pool);
			return MMAP_FILE_NULL;
		}
	}
	
	id = pool->freelist;
	_MP_GET_NODE(pool, id, node);
	pool->freelist = node->next;
	node->used = 1;
	node->next = MMAPPOOL_NULL;

	_mp_unlock(*pool);

	return id; 
}

int 
mmappool_free(mmappool_t *pool, mmappool_id_t id)
{
	int ret = 0;
	mmappool_node_t *node;

	if (unlikely(!pool || id == MMAP_FILE_NULL))
		ERR_RET(-1, "invalid argument\n");

	_mp_lock(pool);

	if (id >= pool->max) {
		_mp_unlock(pool);
		return -1;
	}

	_MP_GET_NODE(p, id, node);
	node->next = pool->freelist;
	pool->freelist = id;

	_mp_unlock(pool);

	return ret;
}

int 
mmappool_copyin(mmappool_t *pool, mmappool_id_t id, const void *p, size_t n)
{
	void *ptr;
	mempool_node_t *node;

	if (unlikely(!pool || id == MMAP_FILE_NULL || !p || n < 1))
		ERR_RET(-1, "invalid argument\n");

	_mp_lock(pool);

	if (id >= pool->max) {
		_mp_unlock(pool);
		return -1;
	}

	node = _MP_GET_NODE(pool, id, node);
	ptr = (void *)node + pool->nodesize;

	memcpy(ptr, p, n);

	_mp_unlock(pool);

	return 0;
}

int 
mmappool_copyout(mmappool_t *pool, mmappool_id_t id, void *p, size_t n)
{
	void *ptr;

	if (unlikely(!pool || id == MMAP_FILE_NULL || !p || n < 1))
		ERR_RET(-1, "invalid argument\n");

	_mp_lock(pool);

	if (id >= pool->max) {
		_mp_unlock(pool);
		return -1;
	}

	node = _MP_GET_NODE(pool, id, node);
	ptr = (void *)node + pool->nodesize;

	memcpy(p, ptr, n);

	_mp_unlock(pool);

	return 0;
}

int  
mmappool_print(const mmappool_t *pool)
{
	if (unlikely(!pool)) 
		ERR_RET(-1, "invalid argument\n");
	
	_mp_lock(pool);
	
	printf("max:     %u\n",  pool->max);
	printf("nfreed:  %u\n",  pool->nfreed);
	printf("objsize: %u\n",  pool->objsize);
	printf("nodesize: %u\n", pool->nodesize);

	_mp_unlock(pool);
	
	return 0;
}


