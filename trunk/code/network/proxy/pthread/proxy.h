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

#include "ip_addr.h"
#include "objpool.h"
#include "thread.h"
#include "session.h"

/* the max work thread number, real server number */
#define MAX_REALSVR	20
#define	MAX_WORKNUM	20
#define	MAX_PKTLEN	4096

typedef struct proxy_stat {
	/* the session information */
	u_int64_t	nhttp;		/* total http session number */
	u_int64_t	nhttps;		/* total https session number */
	u_int64_t	naccept;	/* total session number (http+https) */
	u_int64_t	nhttplive;	/* live http session number */
	u_int64_t	nhttpslive;	/* live https session number */
	u_int64_t	nlive;		/* live session number */

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
	ip_port_t	httpaddr;	/* HTTP server address */
	ip_port_t	httpsaddr;	/* HTTPS server address */
	ip_port_t	rsaddrs[MAX_REALSVR];/* real server address */
	int		nrsaddr;	/* real server number */

	/* the thread infomation */
	thread_t	works[MAX_WORKNUM];/* the parse thread */
	int		nwork;		/* number of work thread */
	thread_t	accept;		/* the accept thread */

	/* the global resource for proxy */
	objpool_t	*pktpool;	/* pool for packet_t */
	objpool_t	*ssnpool;	/* pool for session_t */
	
	u_int32_t	max;		/* max concurrent session*/
	int		stop;		/* proxy stopped */
	int		dbglvl;		/* debug level */
	int		use_splice;	/* use system splice */
	int		use_nb_splice;	/* use nb_splice */
	int		nb_splice_fd;	/* nb_splice fd */
	pthread_t	main_tid;	/* main thread id */
	proxy_stat_t	stat;		/* statistic data */
	pthread_mutex_t	lock;		/* lock for proxy stat */
} proxy_t;

extern proxy_t		g_proxy;


/**
 *	Run a TCP proxy on @ip:@port, the real server is @rip:@rport.
 *	The proxy can process @capacity concurrency connections, the
 *	work thread number is @nworks. If @use_splice is 1, using
 *	splice() to improve performance.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proxy_main(ip_port_t *svraddr, ip_port_t *rsaddrs, int nrsaddr, 
	   int max, int nworks, int use_splice, int use_nb_splice);

/**
 *	lock proxy statistic data.
 *
 *	No return.
 */
extern void 
proxy_stat_lock();

/**
 *	Unlock proxy statistic data.
 *
 *	No return.
 */
extern void 
proxy_stat_unlock();

#endif /* end of FZ_PROXY_H */


