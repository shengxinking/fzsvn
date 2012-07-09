/**
 *	@file:	thread.h
 *
 *	@brief	implement thread create, join, cancel.
 *
 *	@author	Forrest.Zhang
 *
 *	@date	2010-07-06
 */

#ifndef FZ_THREAD_H
#define FZ_THREAD_H

#include <sys/types.h>
#include <pthread.h>

#define FDPAIR_MAX	10240

/* thread type */
#define THREAD_ACCEPT	1	/* accept thread */
#define THREAD_RECV	2	/* recv thread */
#define THREAD_WORK	3	/* work thread */
#define THREAD_PARSE	4	/* work thread */
#define THREAD_SEND	5	/* send thread */
#define THREAD_MAX	5	/* the max type value */

/**
 *	The socket fd and session id pair
 */
typedef struct fd_pair {
	u_int32_t	id;	/* session id */
	int		fd;	/* the socket */
} fd_pair_t;

typedef struct fd_array {
	int		size;
	fd_pair_t	array[FDPAIR_MAX];
	pthread_mutex_t lock;
} fd_array_t;


/**
 *	The thread structure.
 */
typedef struct thread {
	pthread_t	tid;	/* the thread id */
	int		type;	/* see above thread type */
	int		index;	/* only for work thread */	
	void		*priv;	/* for thread private data */
} thread_t;

/**
 *	Create a new thread, the new thread function is @func, 
 *	the @func's argument is @info, the @type see above.
 *
 *	Return 0 if create thread success, -1 on error.
 */
extern int 
thread_create(thread_t *info, int type, void *(*func)(void *arg), int index);

/**
 *	Wait a thread stop it's running.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
thread_join(thread_t *info);

#endif /* end of FZ_THREAD_H */

