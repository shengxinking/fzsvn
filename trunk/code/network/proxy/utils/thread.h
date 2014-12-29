/**
 *	@file:	thread.h
 *
 *	@brief	implement thread create, join, cancel.
 *
 *	@author	Forrest.Zhang
 *
 *	@date	2010-07-06
 */

#ifndef FZ_THREAD1_H
#define FZ_THREAD1_H

#include <sys/types.h>
#include <pthread.h>

enum {
	THREAD_BIND_RR,
	THREAD_BIND_ODD,
	THREAD_BIND_EVEN,
};

enum {
	THREAD_HT_FULL,
	THREAD_HT_LOW,
	THREAD_HT_HIGH,
};

#define	THREAD_STACKSIZE	(2048 * 1024)

/**
 *	The thread structure.
 */
typedef struct thread {
	pthread_t	tid;	/* the thread id */
	int		index;	/* only for work thread */
	void		*cfg;	/* thread config */
	void		*priv;	/* for thread private data */
	void		*stack;	/* the stack for thread */
} thread_t;

/**
 *	Create a new thread, the new thread function is @func, 
 *	the @func's argument is @info, the @type see above.
 *
 *	Return 0 if create thread success, -1 on error.
 */
extern int 
thread_create(thread_t *ti, int index, void *(*func)(void *arg));

/**
 *	Wait a thread stop it's running.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
thread_join(thread_t *ti);

/**
 *	Bind thread @tid to CPU @cpu.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
thread_bind_cpu(pthread_t tid, int index, int algo, int high);

#endif /* end of FZ_THREAD1_H */

