/*
 *	thread.c:	provide thread manager
 *
 *	author:		forrest.zhang
 */

#include <string.h>
#include <errno.h>

#include "thread.h"
#include "debug.h"

/**
 *	Create a new thread, the new thread function is @func, 
 *	the @func's argument is @info, the @type see above.
 *
 *	Return 0 if create thread success, -1 on error.
 */
int 
thread_create(thread_t *info, int type, void *(*func)(void *arg), int index)
{
	if (!info || !func) {
		ERR("thread_create: invalid parameter\n");
		return -1;
	}
	
	info->type = type;
	info->index = index;
	if (pthread_create(&info->tid, NULL, func, info)) {
		ERR("pthread_create: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}


/**
 *	Wait a thread to exit.
 *
 *	Return 0 if success, -1 on error.
 */
int 
thread_join(thread_t *info)
{
	if (!info || !info->tid) {
		return -1;
	}
	
	if (pthread_join(info->tid, NULL)) {
		ERR("pthread_join: %s\n", strerror(errno));
		return -1;
	}

	DBG("thread %lu joined\n", info->tid);

	return 0;
}
