/*
 *	shmpool.c	share memory pool implement
 *
 *	author		forrest.zhang
 */

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/sem.h>

#include "shmpool.h"

void *shmpool_ptr(const shmpool_t *pool, int pos)
{
	if (!pool)
		return NULL;

	if (pos < 0 || pos > (pool->maxobjs - 1) * (pool->objsize + sizeof(shmpool_node_t)))
		return NULL;

	if (pos % (pool->objsize + sizeof(shmpool_node_t)))
		return NULL;

	return ((char *)(pool) + sizeof(shmpool_t) + pos);
}

int shmpool_idx(const shmpool_t *pool, const void *node)
{
	int pos = 0;

	if (!pool || !node)
		return -1;

	pos = ((char *)node - ((char *)pool + sizeof(shmpool_t)));

	if (pos < 0 || pos > (pool->maxobjs - 1) * (pool->objsize + sizeof(shmpool_node_t)))
		return -1;

	if (pos % (pool->objsize + sizeof(shmpool_node_t)))
		return -1;

	return pos;
}

static void _shmpool_init(shmpool_t *pool)
{
	char *buf;
	shmpool_node_t *node;
	int nodesize;
	int i;

	nodesize = pool->objsize + sizeof(shmpool_node_t);
	buf = (char *)pool + sizeof(shmpool_t);
	for (i = 0; i < pool->maxobjs; i++) {
		node = (shmpool_node_t*)(buf + i * nodesize);
		
#ifdef SHMPOOL_VERIFY
		node->magic = pool->magic;
#endif

		node->flags = 0;
		node->size = pool->objsize;
		node->len = 0;
		if (i < pool->maxobjs - 1)
			node->next = (i + 1) * nodesize;
		else
			node->next = -1;
	}
	pool->front = 0;
	pool->rear = (pool->maxobjs - 1) * nodesize;
	pool->freed = pool->maxobjs;
}

shmpool_t *shmpool_alloc(int key, int objsize, int maxobjs)
{
	int shmid;
	int semid;
	int len;
	shmpool_t *pool;
	int val = 0;

	if (objsize < 1 || maxobjs < 0)
		return NULL;

	objsize = ALIGN_4(objsize);
	len = (objsize + sizeof(shmpool_node_t)) * maxobjs + sizeof(shmpool_t);
	shmid = shmget(key, len, IPC_CREAT | IPC_EXCL | 0600);
	if (shmid < 0) {
		printf("shmget error: %s\n", strerror(errno));
		return NULL;
	}

	semid = semget(IPC_PRIVATE, 1, 0600);
	if (shmid < 0) {
		printf("semget error: %s\n", strerror(errno));
		shmctl(shmid, IPC_RMID, NULL);
		return NULL;
	}

	if (semctl(semid, 0, SETVAL, val)) {
		printf("semctl error(SETVAL): %s\n", strerror(errno));
		semctl(semid, 0, IPC_RMID);
		shmctl(shmid, IPC_RMID, NULL);
		return NULL;
	}

	pool = shmat(shmid, NULL, 0);
	if (pool == (void *)-1) {
		printf("shmat error: %s\n", strerror(errno));
		return NULL;
	}

#ifdef SHMPOOL_VERIFY
	pool->magic = time(NULL);
#endif

	pool->shmkey = key;
	pool->shmid = shmid;
	pool->semid = semid;
	pool->objsize = objsize;
	pool->maxobjs = maxobjs;

	_shmpool_init(pool);

	return pool;
}


int shmpool_free(shmpool_t *pool)
{
	int shmid;
	int semid;

	if (pool) {
		shmid = pool->shmid;
		semid = pool->semid;

		if (pool->maxobjs > pool->freed) 
			printf("have %d nodes no return\n", pool->maxobjs - pool->freed);

		if (semctl(semid, 0, IPC_RMID)) {
			printf("semctl error(IPC_RMID): %s\n", strerror(errno));
			return -1;
		}

		if (shmctl(shmid, IPC_RMID, NULL)) {
			printf("shmctl error(IPC_RMID): %s\n", strerror(errno));
			return -1;
		}
	}	

	return 0;
}


shmpool_t *shmpool_attach(int key)
{
	int shmid;
	shmpool_t *pool;

	shmid = shmget(key, 0, 0);
	if (shmid < 0) {
		printf("shmget error: %s\n", strerror(errno));
		return NULL;
	}

	pool = shmat(shmid, NULL, 0);
	if (pool == (void *)-1) {
		printf("shmat error: %s\n", strerror(errno));
		return NULL;
	}

	return pool;
}

int shmpool_detach(shmpool_t *pool)
{
	if (pool) {
		if (shmdt(pool)) {
			printf("shmdt error: %s\n", strerror(errno));
			return -1;
		}
	}
	return 0;
}

shmpool_node_t *shmpool_get(shmpool_t *pool)
{
	struct sembuf buf[2];
	shmpool_node_t *node;

	if (!pool)
		return NULL;

	if (pool->freed == 0)
		return NULL;

	buf[0].sem_num = 0;
	buf[0].sem_op = 0;
	buf[0].sem_flg = 0;
	buf[1].sem_num = 0;
	buf[1].sem_op = 1;
	buf[1].sem_flg = SEM_UNDO;
	if (semop(pool->semid, buf, 2)) {
		printf("semop error(lock): %s\n", strerror(errno));
		return NULL;
	}

	node = shmpool_ptr(pool, pool->front);
	pool->front = node->next;
	if (pool->front == -1) {
		pool->rear = -1;
	}
	node->next = -1;
	node->flags = SHMPOOL_USED;
	pool->freed--;

	buf[0].sem_num = 0;
	buf[0].sem_op = -1;
	buf[0].sem_flg = SEM_UNDO;
	if (semop(pool->semid, buf, 1)) {
		printf("semop error(unlock): %s\n", strerror(errno));
		return NULL;
	}

	return node;
}

int shmpool_put(shmpool_t *pool, shmpool_node_t *node)
{
	struct sembuf buf[2];
	shmpool_node_t *rear = NULL;

	if (!pool || !node)
		return -1;

	if (!(node->flags & SHMPOOL_USED))
		return -1;

#ifdef SHMPOOL_VERIFY
	if (pool->magic != node->magic)
		return -1;
#endif

	buf[0].sem_num = 0;
	buf[0].sem_op = 0;
	buf[0].sem_flg = 0;
	buf[1].sem_num = 0;
	buf[1].sem_op = 1;
	buf[1].sem_flg = SEM_UNDO;
	if (semop(pool->semid, buf, 2)) {
		printf("semop error(lock): %s\n", strerror(errno));
		return -1;
	}

	node->next = -1;
	node->flags = 0;
	if (pool->freed < 1) {
		pool->rear = pool->front = shmpool_idx(pool, node);
	}
	else {
		rear = shmpool_ptr(pool, pool->rear);
		rear->next = shmpool_idx(pool, node);
		pool->rear = shmpool_idx(pool, node);
	}
	pool->freed++;

	buf[0].sem_num = 0;
	buf[0].sem_op = -1;
	buf[0].sem_flg = SEM_UNDO;
	if (semop(pool->semid, buf, 1)) {
		printf("semop error(unlock): %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

void shmpool_print(const shmpool_t *pool)
{
	int nodesize;
	shmpool_node_t *node;
	char *buf;
	int i;

	if (pool) {
#ifdef SHMPOOL_VERIFY
		printf("pool: magic %d, shmkey %d, shmid %d, semid %d, objsize %d, maxobjs %d, freed %d\n",
			pool->magic, pool->shmkey, pool->shmid, 
			pool->semid, pool->objsize, pool->maxobjs, pool->freed);
#else
		printf("pool: shmkey %d, shmid %d, semid %d, objsize %d, maxobjs %d, freed %d\n",
			pool->shmkey, pool->shmid, pool->semid, pool->objsize, pool->maxobjs, pool->freed);
#endif

		nodesize = pool->objsize + sizeof(shmpool_node_t);
		buf = (char *)pool + sizeof(shmpool_t);
		printf("pool %p, buf %p, front %d, rear %d\n", pool, buf, pool->front, pool->rear);
		i = pool->front;
		while (i != -1) {
			node = shmpool_ptr(pool, i);
			printf("\tnode %p, pos %d, size %d, len %d, next %d\n", node, i, node->size, node->len, node->next);
			i = node->next;
		}
	}
}




