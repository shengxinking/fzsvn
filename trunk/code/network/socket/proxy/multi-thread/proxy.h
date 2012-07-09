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
#include "session.h"
#include "mempool.h"

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
	u_int64_t	nhttp;		/* http session number(total) */
	u_int64_t	nhttps;		/* https session number(total) */
	u_int64_t	naccept;	/* total session number (http+https) */
	u_int64_t	nhttplive;	/* live http session number */
	u_int64_t	nhttpslive;	/* live https session number */
	u_int64_t	nlive;		/* live session number (http + https) */

	/* the flow information */
	u_int64_t	nclirecv;	/* recv client bytes */
	u_int64_t	nclisend;	/* send client bytes */
	u_int64_t	nsvrrecv;	/* recv server bytes */
	u_int64_t	nsvrsend;	/* send server bytes */

	/* the close/error information */
	u_int64_t	ncliclose;	/* client close times */
	u_int64_t	nclierror;	/* client error times */
	u_int64_t	nsvrclose;	/* server close times */
	u_int64_t	nsvrerror;	/* server error times */
} proxy_stat_t;


/**
 *	global proxy struct
 */
typedef struct proxy {

	/* the proxy address and real server address */
	u_int32_t	ip;		/* proxy address */
	u_int32_t	rip;		/* real server address */
	u_int16_t	port;		/* HTTP port */
	u_int16_t	rport;		/* real server port */
	u_int16_t	sport;		/* HTTPS port */
	u_int16_t	rsport;		/* real server HTTPS port */

	/* the thread infomation */
	thread_t	works[WORK_MAX];/* the parse thread */
	int		nworks;		/* the number of work thread */
	thread_t	accept;		/* the accept thread */

	/* the global resource for proxy */
	mempool_t	*pktpool;	/* memory for recv data */
	sesstbl_t	*sesstbl;	/* TCP session table */
	sessmap_t	*sessmap;	/* the session map id:session */
	
	u_int32_t	capacity;	/* the max concurrent connections */
	int		stop;		/* control proxy to exist */
	pthread_t	main_tid;	/* the main thread id of proxy */
	proxy_stat_t	stat;		/* the proxy status */
	pthread_mutex_t	lock;		/* the lock for proxy stat */
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
proxy_main(u_int32_t ip, u_int16_t port, u_int16_t sport, 
	   u_int32_t rip, u_int16_t rport, u_int16_t rsport, 
	   int capacity, int nworks);

#endif /* end of FZ_PROXY_H */


