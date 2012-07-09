/**
 *	@file	proxy.h
 *
 *	@brief	the proxy header file.
 *
 *	@date	2008-07-30
 */

#ifndef FZ_PROXY_H
#define FZ_PROXY_H

#include <sys/types.h>
#include <pthread.h>

#include "thread.h"

/* the max work thread number */
#define WORK_MAX	20

/**
 * the max packet size, a packet is a cache node which used to
 * recv data 
 */
#define PKT_SIZE	(1024 * 2)

#define FD_MAX		(1024 * 10)

typedef struct proxy_stat {

	/* the session information */
	u_int64_t	naccepts;	/* accept client */

	/* the flow information */
	u_int64_t	nclirecvs;	/* recv client bytes */
	u_int64_t	nclisends;	/* send client bytes */
	u_int64_t	nsvrrecvs;	/* recv server bytes */
	u_int64_t	nsvrsends;	/* send server bytes */
	u_int64_t	ncliparses;	/* parse client bytes */
	u_int64_t	nsvrparses;	/* parse server bytes */

	/* the error information */
	u_int64_t	nclicloses;	/* client close times */
	u_int64_t	nclierrors;	/* client error times */
	u_int64_t	nsvrcloses;	/* server close times */
	u_int64_t	nsvrerrors;	/* server error times */
} proxy_stat_t;

struct mempool;
struct sesspool;

/**
 *	global proxy struct
 */
typedef struct proxy {

	/* the proxy address and real server address */
	u_int32_t	ip;		/* proxy address */
	u_int32_t	rip;		/* real server address */
	u_int16_t	port;		/* proxy port */
	u_int16_t	rport;		/* real server port */

	/* the thread infomation */
	thread_t	works[WORK_MAX];/* the work thread */
	thread_t        recvs[WORK_MAX];/* the recv thread */
	thread_t	sends[WORK_MAX];/* the send thread */
	int		nworks;		/* the number of parse thread */
	thread_t	accept;		/* the accept thread */

	/* the global resource for proxy */
	struct mempool	*pktpool;	/* get memory for recv data */
	struct sesspool	*sesspools[WORK_MAX];	/* sesspool is TCP session table */
	
	u_int16_t	capacity;	/* the max concurrent connections */
	int		stop;		/* control proxy to exist */
	pthread_t	main_tid;	/* the main thread id of proxy */
	proxy_stat_t	stat;		/* the proxy status */	
} proxy_t;

extern proxy_t		g_proxy;

/**
 *	Run a TCP proxy on @ip:@port, the real server is @rip:@rport.
 *	The proxy can process @capacity concurrency connections, the
 *	work thread number is @nworks.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proxy_main(u_int32_t ip, u_int16_t port, u_int32_t rip, u_int16_t rport,
	   int capacity, int nworks);

#endif /* end of FZ_PROXY_H */


