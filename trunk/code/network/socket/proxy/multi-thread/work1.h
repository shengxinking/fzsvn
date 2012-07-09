/**
 *	@file	parse.h
 *
 *	@brief	the parse thread, it recved the 
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_WORK_H
#define FZ_WORK_H

#include <sys/types.h>

#include "thread.h"
#include "http_protocol.h"
#include "packet.h"

typedef enum {
	ACTION_ACCEPT = 0,
	ACTION_DENY,
	ACTION_CACHE,
} parse_action;


/**
 *	The resource for parse.
 */
typedef struct _work_session {
	u_int32_t	sid;
	pktqueue_t	cli2svr;
	pktqueue_t	svr2cli;
	http_info_t	*http;
} work_session_t;


/**
 *	The work struct.
 */
typedef struct _work {	
	pktqueue_t	inq;
	pktqueue_t	pq;
	fd_array_t	delfds;
	work_session_t	*sess;
	u_int32_t	nsess;

	pthread_mutex_t	lock;
} work_t;


/**
 *	The work thread main function.
 *
 *	No return.
 */
extern void *
work_run(void *arg);


/**
 *	Add a new packet to work thread @index.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
work_add_pkt(int index, packet_t *pkt);


/**
 *	Delete a session from work thread @index.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
work_del_id(int index, u_int32_t id);


#endif /* end of FZ_WORK_H  */

