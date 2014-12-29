/**
 *	@file	session.h
 *
 *	@brief	The Session Table implement.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2010-02-23
 */

#ifndef FZ_SESSION_H
#define FZ_SESSION_H

#include "proxy.h"
#include "thread.h"
#include "packet.h"
#include "ip_addr.h"
#include "cblist.h"
#include "task.h"
#include "worker.h"

/* the flags in session_data */
#define	SESSION_SHUTRD		0x0001
#define	SESSION_SHUTWR		0x0002
#define	SESSION_SHUTDOWN	(SESSION_SHUTRD|SESSION_SHUTWR)
#define	SESSION_BLOCKED		0x0004
#define	SESSION_HANDSHAKE	0x0008

struct session;

/**
 *	The session data, it run in task to handle like
 *	handshake/recv/send/parse data.
 */
typedef struct session_data {
	int		fd;		/* socket fd */
	ip_port_t	addr;		/* peer address */
	cblist_t	in;		/* input queue */
	cblist_t	out;		/* output queue */
	struct session_data *peer;	/* the other side of proxy */
	const char	*side;		/* the string for debug */
	int		flags;		/* flags */
	task_t		task;		/* tasks for handle data */
} session_data_t;

struct task;

/**
 *	Session structure.
 */
typedef struct session {
	u_int32_t	id;		/* session id */

	proxy_t		*proxy;		/* session belong proxy */
	thread_t	*thread;	/* session belong thread */
	worker_t	*worker;	/* session belong worker */
	policy_t	*policy;	/* session belong policy */

	cblist_t	list;		/* in session list */

	session_data_t	clidata;	/* client data */
	session_data_t	svrdata;	/* server data */

	int		rd_pipe;	/* read pipe for splice */
	int		wr_pipe;	/* write pipe for splice */

	int		nalloced;	/* alloced packet number */
	int		flags;		/* the flags of session */
} session_t;

static inline void 
session_data_init(session_t *s, session_data_t *sd, 
		session_data_t *peer, const char *side)
{
	CBLIST_INIT(&sd->in);
	CBLIST_INIT(&sd->out);
	sd->fd = -1;
	sd->peer = peer;
	sd->side = side;
	sd->flags = 0;
	task_init(&sd->task);
	sd->task.arg1 = s;
	sd->task.arg2 = sd;
	sd->task.peer = &peer->task;
}

static inline 
void session_init(session_t *s)
{
	/* init common value */
	s->rd_pipe = -1;
	s->wr_pipe = -1;
	s->nalloced = 0;
	s->flags = 0;

	session_data_init(s, &s->clidata, &s->svrdata, "client");
	session_data_init(s, &s->svrdata, &s->clidata, "server");

	CBLIST_INIT(&s->list);
}

/**
 *	Print the session information.
 *
 *	No return.
 */
extern void 
session_print(const session_t *s);

#endif /* end of FZ_SESSION_H  */

