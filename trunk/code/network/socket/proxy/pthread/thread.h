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

#include "ip_addr.h"
#include "packet.h"
#include "session.h"
#include "objpool.h"

#define	ACCEPT_TIMEOUT	100
#define	WORK_TIMEOUT	100

/* thread type */
#define THREAD_ACCEPT	1	/* accept thread */
#define THREAD_RECV	2	/* recv thread */
#define THREAD_WORK	3	/* work thread */
#define THREAD_PARSE	4	/* work thread */
#define THREAD_SEND	5	/* send thread */
#define THREAD_MAX	5	/* the max type value */

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
 *	The private data of accept thread.
 */
typedef struct _accept {
	int		httpfd;		/* HTTP listen fd */
	int		httpsfd;	/* HTTPS listen fd */
	int		index;		/* the next work index */
} accept_t;

typedef struct _workfd {
	int		fd;		/* client fd */
	u_int32_t	is_ssl:1;	/* is ssl */
} workfd_t;

/**
 *	The private data of work_epoll thread.
 */
typedef struct _work {
	workfd_t	*inq;		/* input queue for accept */
	workfd_t	*inq2;		/* secondary input queue */
	pthread_mutex_t	lock;		/* lock for input queue */
	int		ninq;		/* number of objs in input queue */
	int		max;		/* max obj in input queue */

//	objpool_t	*pktpool;	/* packet_t pool */
//	objpool_t	*ssnpool;	/* session_t pool */
	session_table_t	*sesstbl;	/* session table for each thread */
	int		send_epfd;	/* send epoll */
	int		recv_epfd;	/* recv epoll */
	struct epoll_event *events;	/* events for epoll */
	int		nevent;		/* number of event */
	int		nblocked;	/* number of blocked */
} work_t;

/**
 *	The private data for work thread.
 */
typedef struct _parse {
	pktq_t		inq;
	pthread_mutex_t	lock;
} parse_t;

/**
 *	The private data for recv thread
 */
typedef struct _recv {
	session_t	**inq;		/* input queue */
session_t	**inq2;		/* secondary input queue */
pthread_mutex_t	lock;		/* lock for input queue */
	int		nfd;		/* number obj in input queue */
int		max;		/* input queue max */

struct epoll_event *events;
} recv_t;

/**
 *	The private data for send thread.
 */
typedef struct _send {
	pktq_t		inq;
	pthread_mutex_t	lock;

	pktq_t		blkq;
	struct epoll_event *events;
} send_t;

/**
 *	Create a new thread, the new thread function is @func, 
 *	the @func's argument is @info, the @type see above.
 *
 *	Return 0 if create thread success, -1 on error.
 */
extern int 
thread_create(thread_t *i, int t, void *(*func)(void *arg), int index);

/**
 *	Wait a thread stop it's running.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
thread_join(thread_t *info);

/**
 *	The main function of accept thread.
 *
 *	Always return NULL.
 */
extern void * 
accept_run(void *arg);

/**
 *	The main function of work thread.
 *
 *	Always return NULL.
 */
extern void *
work_run(void *arg);

/**
 *	Add a new session into work thread.
 *
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
work_add_session(int index, workfd_t *wfd);

/**
 *	The main function of work thread
 */
extern void *
parse_run(void *arg);

/**
 *	The main function of recv thread
 *
 *	Always return NULL.
 */
extern void *
recv_run(void *arg);

/**
 *	The main function of send thread.
 *
 *	Always return NULL.
 */
extern void *
send_run(void *arg);

#endif /* end of FZ_THREAD_H */

