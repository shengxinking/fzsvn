/*
 *	shmpool.h	interface of share memory pool, it's memory pool, that process can get memory from it, and store
 *			data in it and share with other process.
 *
 *	author		forrest.zhang
 */

#ifndef FZ_SHMPOOL_H
#define FZ_SHMPOOL_H

#include <sys/ipc.h>
#include <sys/shm.h>

#ifndef ALIGN_4
#define ALIGN_4(n)      (((n - 1) / 4 + 1) * 4)
#endif

#define SHMPOOL_USED	0x1

typedef struct shmpool_node {
	int		next;
	int		flags;

#ifdef SHMPOOL_VERIFY
	int		magic;
#endif

	int		size;
	int		len;
	char		data[0];
} shmpool_node_t;

typedef struct shmpool {

#ifdef SHMPOOL_VERIFY
	int		magic;
#endif

	int		shmkey;
	int		shmid;
	int		semid;
	int		objsize;
	int		maxobjs;
	int		freed;
	int		front;
	int		rear;
} shmpool_t;	

extern shmpool_t *shmpool_alloc(int key, int objsize, int maxobjs);
extern int shmpool_free(shmpool_t *pool);

extern shmpool_t *shmpool_attach(int key);
extern int shmpool_detach(shmpool_t *pool);

extern shmpool_node_t *shmpool_get(shmpool_t *pool);
extern int shmpool_put(shmpool_t *pool, shmpool_node_t *node);

extern void *shmpool_ptr(const shmpool_t *pool, int pos);
extern int shmpool_idx(const shmpool_t *pool, const void *node);

extern void shmpool_print(const shmpool_t *pool);

#endif /* end of FZ_SHMPOOL_H */

