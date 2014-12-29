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
#include "thread.h"
#include "task.h"

/**
 *	The private data of worker thread.
 */
typedef struct worker {
	u_int32_t	next_sid;	/* the next session id */
	int		naccept;	/* accept number in one time */
	objpool_t	*pktpool;	/* packet_t pool */
	objpool_t	*ssnpool;	/* session_t pool */
	fd_epoll_t	*fe;		/* the fd epoll object */
	task_queue_t	*taskq;		/* the task queue */

	cblist_t	lfdlist;	/* listener_fd_t list */
	int		nlfd;		/* number of listener_fd_t */

	cblist_t	cmdlist;	/* command list */
	int		ncmd;

	pthread_mutex_t	lock;		/* command list lock */
} worker_t;

/**
 *	worker thread control command type.
 */
typedef enum {
	WORKER_CMD_ADD_POLICY,
	WORKER_CMD_DEL_POLICY,
	WORKER_CMD_MAX,
} worker_cmd_e;

/**
 *	worker thread control command, alloced in proxy.c
 *	and need free in worker thread.
 */
typedef struct worker_cmd {
	cblist_t	list;	/* list into work_t's @cmdlist */ 
	worker_cmd_e	cmd;	/* command type */
	void		*arg;	/* policy_t */
} worker_cmd_t;

/**
 *	The main function of work thread.
 *
 *	Always return NULL.
 */
extern void *
worker_run(void *arg);

/**
 *	Add a worker command @cmd into worker thread @ti.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
worker_add_cmd(thread_t *ti, worker_cmd_t *cmd);

#endif /* end of FZ_WORKER_H */


