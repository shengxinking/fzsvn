/*
 *	thread.c:	provide thread manager
 *
 *	author:		forrest.zhang
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "thread.h"

/**
 *	Define macro to print error message
 */
#define _THREAD_DBG
#ifdef	_THREAD_DBG
#define _THREAD_ERR(fmt, args...)	printf("thread:%s:%d: " fmt, \
				       __FILE__, __LINE__, ##args)
#else
#define _THREAD_ERR(fmt, args...)
#endif

/**
 *	Create a posix thread, and set the thread infomation to @info, include
 *	@type, @thread_id. the thread function is @func.
 *
 *	Return 0 if create success, -1 on error.
 */
int 
thread_create(thread_t *info, int type, void *(*func)(void *arg))
{
	if (!info || !func) {
		_THREAD_ERR("thread_create: invalid parameter\n");
		return -1;
	}
	
	info->type = type;
	if (pthread_create(&info->tid, NULL, func, info)) {
		_THREAD_ERR("pthread_create: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

/**
 *	Wait a thread to exit, if the thread is running now, it'll wait until
 *	the thread is exit.
 *
 *	Return 0 if success, -1 on error.
 */
int 
thread_join(thread_t *info)
{
	if (!info) {
		_THREAD_ERR("thread_join: invalid parameter\n");
		return -1;
	}
	
	if (!info->tid)
		return 0;

	if (pthread_join(info->tid, NULL)) {
		_THREAD_ERR("pthread_join: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}


/**
 *	Print thread @info infomation.
 *
 *	No return.
 */
void 
thread_print(const thread_t *info)
{
	if (!info)
		return;

	printf("thread %lu: type %d\n", info->tid, info->type);
}

