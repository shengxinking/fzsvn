/**
 *	@file	worker.h
 *
 *	@brief	The worker data structure.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2014-03-14
 */

#ifndef FZ_WORKER_H
#define FZ_WORKER_H

#include "objpool.h"
#include "fd_epoll.h"
#include "proxy.h"
#include "task.h"

/**
 *	The private data of work_epoll thread.
 */
typedef struct worker {
	u_int32_t	next_sid;	/* the next session id */
	struct policy_fd pfds[MAX_POLICY];/* the policies */
	int		npfd;		/* number of policies */
	objpool_t	*pktpool;	/* packet_t pool */
	objpool_t	*ssnpool;	/* session_t pool */
	fd_epoll_t	*fe;		/* the fd epoll object */
	task_queue_t	*taskq;		/* the task queue */
	struct timeval	timestamp;	/* the timestamp */
} worker_t;

/**
 *	The main function of work thread.
 *
 *	Always return NULL.
 */
extern void *
worker_run(void *arg);

#endif /* end of FZ_WORKER_H */


