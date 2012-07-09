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

#include "mempool.h"
#include "session.h"

#define PROXY_TIMEOUT		100
#define PROXY_PKTSIZE		1024


typedef struct proxy_stat {
	u_int64_t	nconnection;	/* number of TCP connections */
	u_int64_t	nconcurrency;	/* number of TCP concurrency */
} proxy_stat_t;

typedef struct proxy {
	int		listenfd;	/* listener fd */
	int		send_epfd;	/* send epoll fd */
	int		recv_epfd;	/* recv epoll fd */
	u_int32_t	ip;		/* the server's ip */
	u_int32_t	rip;		/* the real server's ip */
	u_int16_t	port;		/* the server's port */
	u_int16_t	rport;		/* the real server's port */
	struct epoll_event *events;	/* used for epoll_wait() */
	mempool_t	*pktpool;	/* memory pool for packet */
	mempool_t	*ssnpool;	/* memory pool for session */
	session_table_t	*sessions;	/* session table */
	int		nblocked;	/* number of send blocked socket */
	int		capacity;	/* proxy capacity, how many concurrent 
					   connections in proxy */
	proxy_stat_t	stat;		/* current connections in proxy */
} proxy_t;


/**
 *	the main entry of proxy, it listen on @ip:@port, and the real server
 *	is @rip:@rport.
 *
 *	return 0 if success, -1 on error.
 */
extern 
int proxy_main(u_int32_t ip, u_int16_t port, 
	       u_int32_t rip, u_int16_t rport, 
	       int capacity);

#endif /* end of FZ_PROXY_H */

