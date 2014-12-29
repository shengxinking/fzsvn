/**
 *	@file	dashboard.h
 *
 *	@brief	The dash board APIs, it use share memory to store a common 
 *		data, like statistic data, daemon information.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2010-10-20
 */

#ifndef FZ_DASHBOARD_H
#define FZ_DASHBOARD_H

#include <pthread.h>
#include <sys/shm.h>
#include <sys/types.h>

typedef struct dashboard {
	int		size;
	int		shmid;
	pthread_mutex_t	lock;
	char		data[0];
} dashboard_t;

/**
 *	Alloc a dashboard for process.
 *
 *	Return a pointer to dashboard if success, NULL on error.
 */
extern dashboard_t * 
dbd_alloc(key_t key, u_int32_t size);


/**
 *	Free a dashboard @dbd.
 *
 *	Return 0 if free success, NULL on error.
 */
extern int 
dbd_free(dashboard_t *dbd);

/**
 *	Attach a dashboard object and return it.
 *
 *	Return a pointer to dashboard object if success, NULL on error.
 */
extern dashboard_t *  
dbd_attach(key_t key);


/**
 *	Detach a dashboard object @dbd, the pointer is invalid after call.
 *
 *	Return 0 is detach success, -1 on error.
 */
extern int 
dbb_detach(dashboard_t *dbd);

/**
 *	Lock the dashboard object @dbd.
 *
 *	Return 0 if lock success, -1 on error.
 */
extern int 
dbd_lock(dashboard_t *dbd);


/**
 *	Unlock the dashboard object @dbd.
 *
 *	Return 0 if unlock success, -1 on error.
 */
extern int 
dbd_unlock(dashboard_t *dbd);


/**
 *	The cleanup function for dashboard, it using atexit()	
 *	function and avoid crash/quit when process get lock but
 *	not unlock.
 *
 *	return 0 if add cleanup function success, -1 on error.
 */
extern int 
dbd_cleanup(dashboard_t *dbd);


#endif /* end of FZ_DASHBOARD_H  */

