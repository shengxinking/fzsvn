/**
 *	@file	session.h
 *
 *	@brief	The proxy session structure and APIs
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_SESSION_H
#define FZ_SESSION_H

#include "cblist.h"
#include "task.h"
#include "connection.h"

/**
 *	Session direction 
 */
typedef enum session_dir {
	SESSION_DIR_REQ,	/* request direction */	
	SESSION_DIR_RES,	/* response direction */
} session_dir_e;

/**
 *	Session proccess function type
 */
struct session;
typedef int (*session_func)(struct session *s, session_dir_e dir);

typedef struct session {
	u_int32_t	sid;		/* session id for debug */
	connection_t	conns[2];	/* 2 connections */
	cblist_t	lfd;		/* list to listener_fd list */
	cblist_t	svr;		/* list to server list */
	int		nalloced;	/* alloced packet */

	cblist_t	request;	/* request packet list */
	cblist_t	response;	/* response packet list */

	void		*worker;	/* worker_t */
	void		*thread;	/* thread */
	void		*policy;	/* policy */
	void		*svrdata;	/* server data */

	session_func	fparse_func;	/* fast parse function */
	session_func	getsvr_func;	/* server loadbalance function */
	session_func	parse_func;	/* normal parse function */
	session_func	forward_func;	/* forward function */
} session_t;

/**
 *	Init session @s
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
session_init(session_t *s);

/**
 *	Free session @s.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
session_free(session_t *s, connection_t *c);

/**
 *	Session run task.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
session_run_task(task_t *t);

/**
 *	Session @s fast parse data in connection @c->in
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
session_fparse(session_t *s, connection_t *c);

/**
 *	Session @s full parse data in @request/@response.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
session_parse(session_t *s, connection_t *c);

/**
 *	Session @s forward data in @request/@response into 
 *	connection @c->out
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
session_forward(session_t *s, connection_t *c);


#endif /* end of FZ_SESSION_H  */


