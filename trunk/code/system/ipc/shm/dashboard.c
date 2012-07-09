/**
 *	@file	dashboard.c
 *
 *	@brief	The dashboard implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2010-10-20
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "dashboard.h"

/**
 *	Alloc a dashboard for process.
 *
 *	Return a pointer to dashboard if success, NULL on error.
 */
dashboard_t * 
dbd_alloc(key_t key, u_int32_t size)
{
	dashboard_t *dbd = NULL;
	int shmid;
	size_t len;

	len = size + sizeof(dashboard_t);
	printf("len is %d, key is 0x%x\n", len, key);
	shmid = shmget(key, len, 0644 | IPC_CREAT);
	if (shmid < 0) {
		printf("shmget failed: %s\n", strerror(errno));
		return NULL;
	}

	dbd = shmat(shmid, NULL, 0);
	if (!dbd) {
		return NULL;
	}

	dbd->shmid = shmid;
	pthread_mutex_init(&dbd->lock, NULL);
	dbd->size = size;

	return dbd;
}


/**
 *	Free a dashboard @dbd.
 *
 *	Return 0 if free success, NULL on error.
 */
int 
dbd_free(dashboard_t *dbd)
{
	if (!dbd)
		return -1;

	if (shmctl(dbd->shmid, IPC_RMID, NULL)) {
		return -1;
	}
	
	return 0;
}


/**
 *	Attach a dashboard object and return it.
 *
 *	Return a pointer to dashboard object if success, NULL on error.
 */
dashboard_t *  
dbd_attach(key_t key)
{
	int shmid;
	dashboard_t *dbd;

	shmid = shmget(key, 0, 0644);
	if (!shmid)
		return NULL;

	dbd = shmat(shmid, NULL, 0);
	if (!dbd)
		return NULL;
	
	if (shmid != dbd->shmid) {
		shmdt(dbd);
		return NULL;
	}

	return dbd;
}


/**
 *	Detach a dashboard object @dbd, the pointer is invalid after call.
 *
 *	Return 0 is detach success, -1 on error.
 */
int 
dbb_detach(dashboard_t *dbd)
{
	int ret;

	if (!dbd)
		return -1;

	ret = shmdt(dbd);

	return ret;
}


/**
 *	Lock the dashboard object @dbd.
 *
 *	Return 0 if lock success, -1 on error.
 */
int 
dbd_lock(dashboard_t *dbd)
{
	int ret = 0;

	if (!dbd)
		return -1;

	ret = pthread_mutex_lock(&dbd->lock);

	return ret;
}


/**
 *	Unlock the dashboard object @dbd.
 *
 *	Return 0 if unlock success, -1 on error.
 */
int 
dbd_unlock(dashboard_t *dbd)
{
	int ret = 0;

	if (!dbd)
		return -1;

	ret = pthread_mutex_unlock(&dbd->lock);

	return ret;
}







