/**
 *	@file	proxy.h
 *	@brief	head file of proxy.c. proxy.c is a single process TCP proxy APIs.
 *
 *	@author	Forrest.zhang.
 */


#ifndef FZ_PROXY_H
#define FZ_PROXY_H

#include <sys/types.h>
#include <sys/epoll.h>

#include "objpool.h"
#include "session.h"

#define PROXY_TIMEOUT		100
#define PROXY_PKTSIZE		1024
#define	MAX_RS			20

typedef struct proxy_stat {
	u_int64_t	nconn;		/* number of TCP connections */
	u_int64_t	nliveconn;	/* number of live TCP connections */
} proxy_stat_t;

typedef struct proxy {
	int		listenfd;	/* listener fd */
	int		send_epfd;	/* send epoll fd */
	int		recv_epfd;	/* recv epoll fd */
	ip_port_t	svraddr;	/* listen address */
	ip_port_t	rsaddrs[MAX_RS];/* real server address */
	int		nrsaddr;	/* number of realserver */
	struct epoll_event *events;	/* used for epoll_wait() */
	objpool_t	*pktpool;	/* memory pool for packet */
	objpool_t	*ssnpool;	/* memory pool for session */
	objpool_t	*evpool;	/* event pool for epoll event */
	session_table_t	*sessions;	/* session table */
	int		nblocked;	/* number of send blocked socket */
	int		capacity;	/* proxy capacity, how many concurrent 
					   connections in proxy */

	ep_ctx_t	*epctx;		/* epoll event context for proxy_event.c */
	ep_event_t	acceptev;	/* accept event */

	int		use_splice;	/* use splice */
	int		use_nb_splice;	/* use nb_splice */
	int		splice_fd;	/* nb_splice fd */

	proxy_stat_t	stat;		/* current connections in proxy */
} proxy_t;


/**
 *	the main entry of proxy, it listen on @ip:@port, and the real server
 *	is @rip:@rport.
 *
 *	return 0 if success, -1 on error.
 */
extern 
int proxy_main(ip_port_t *svraddr, ip_port_t *rsaddrs, int nrsaddr, 
	       int max, int use_splice, int use_nb_splice); 


#endif /* end of FZ_PROXY_H */

