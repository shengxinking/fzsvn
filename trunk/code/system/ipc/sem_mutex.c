/**
 *	@file	sem_mutex.c
 *
 *	@brief	Using semphore implement a mutex.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2012-07-26
 */


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>


int 
sem_mutex_create(int key)
{
	int id;

	/* create a semphore using @id */
	id = semget(key, 1, IPC_CREAT | IPC_EXCL | 0644);
	if (id < 0) {
		return -1;
	}

	/* init the semphore */
	if (semctl(id, 0, SETVAL, 1)) {
		return -1;
	}

	return 0;
}

int 
sem_mutex_destroy(int key)
{
	int id;

	id = semget(key, 1, 0);
	if (id < 0)
		return -1;

	if (semctl(id, 0, IPC_RMID))
		return -1;

	return 0;
}

int 
sem_mutex_lock(int key)
{
	struct sembuf sb;	
	int id;

	id = semget(key, 1, 0);
	if (id < 0)
		return -1;

	memset(&sb, 0, sizeof(sb));
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;

	if (semop(id, &sb, 1)) {
		return -1;
	}

	return 0;
}


int 
sem_mutex_unlock(int key)
{
	struct sembuf sb;
	int id;

	id = semget(key, 1, 0);
	if (id < 0)
		return -1;

	memset(&sb, 0, sizeof(sb));
	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = SEM_UNDO;

	if (semop(id, &sb, 1)) {
		return -1;
	}

	return 0;
}


int 
sem_mutex_print(int key)
{
	pid_t pid;
	int val;
	int id;

	id = semget(key, 1, 0);
	if (id < 0)
		return -1;

	val = semctl(id, 0, GETVAL);
	if (val < 0)
		return -1;
	
	pid = semctl(id, 0, GETPID);
	if (pid < 0)
		return -1;

	printf("val is %d, pid is %d\n", val, pid);

	return 0;
}
