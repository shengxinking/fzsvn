/*
 *	@file:	thread.h
 *
 *	@brief	implement thread create, join, cancel.
 *
 */

#ifndef FZ_THREAD_H
#define FZ_THREAD_H

#include <pthread.h>

/* thread type */
#define THREAD_RECV	1
#define THREAD_WORK	2
#define THREAD_CLOCK	3
#define THREAD_MONITOR	4
#define THREAD_PSSTC    5
#define THREAD_STATUS   6


typedef struct thread {
	pthread_t	tid;
	int		type;	/* see above thread type */
	int		index;	/* only for work thread */
	
	void		*priv;	/* for thread private data */

} thread_t;

/**
 *	Create a posix thread
 *
 *	Return 0 if create success, -1 on error.
 */
extern int 
thread_create(thread_t *info, int type, void *(*func)(void *arg));

/**
 *	Wait a posix thread exit. It'll wait if the thread @info is not exit
 *	immediately, the the thread is not exist, it'll return -1.
 *	
 *	Return 0 if success, -1 on error.
 */
extern int 
thread_join(thread_t *info);


/**
 *	Print the thread @info on screen.
 *
 *	No return.
 */
extern void 
thread_print(const thread_t *info);

#endif /* end of FZ_THREAD_H */

