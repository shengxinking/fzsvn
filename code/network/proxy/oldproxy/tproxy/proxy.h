/**
 *	@file	proxy.h
 *
 *	@brief	the proxy structure and APIs.
 *
 */

#ifndef	FZ_PROXY_H
#define	FZ_PROXY_H

#define	MAX_REALSVR	1024
#define	MAX_POLICY	32
#define	MAX_WORKER	128
#define	MAX_PKTLEN	(4096 - sizeof(packet_t))

#include <sys/types.h>

#include "objpool.h"
#include "ip_addr.h"
#include "thread.h"


typedef struct proxy_stat {
	/* the session information */
	u_int64_t       nhttp;          /* total http session number */
	u_int64_t       nhttps;         /* total https session number */
	u_int64_t       naccept;        /* total session number */
	u_int64_t       nhttplive;      /* live http session number */
	u_int64_t       nhttpslive;     /* live https session number */
	u_int64_t       nlive;          /* live session number */

	/* the flow information */
	u_int64_t       nclirecv;       /* recv client bytes */
	u_int64_t       nclisend;       /* send client bytes */
	u_int64_t       nsvrrecv;       /* recv server bytes */
	u_int64_t       nsvrsend;       /* send server bytes */

	/* the close/error information */
	u_int64_t       ncliclose;      /* client close times */
	u_int64_t       nclierror;      /* client error times */
	u_int64_t       nsvrclose;      /* server close times */
	u_int64_t       nsvrerror;      /* server error times */
} proxy_stat_t;


typedef struct policy {
	int		index;		/* policy index */
	ip_port_t	httpaddr;	/* HTTP listen address */
	ip_port_t	httpsaddr;	/* HTTPS listen address */
	ip_port_t	rsaddrs[MAX_REALSVR];/* real server */
	int		nrsaddr;	/* number of real server */
	int		pos;		/* the RR pos for loadbalance */
} policy_t;

struct proxy;
struct thread;
struct work;
/**
 *	Policy listen fd structure, used in fd_epoll 
 *	callback function.
 */
typedef struct policy_fd {
	int		httpfd;
	int		httpsfd;
	struct policy	*policy;
	struct thread	*thread;
	struct worker	*worker;
} policy_fd_t;

typedef struct proxy {
	policy_t	policies[MAX_POLICY];/* policies in proxy */
	int		npolicy;
	
	thread_t	workers[MAX_WORKER];
	int		nworker;

	objpool_t	*pktpool;
	objpool_t	*ssnpool;

	int		max;
	int		maxfd;

	int		use_splice;
	int		use_nb_splice;
	int		nb_splice_fd;

	int		bind_cpu;
	int		bind_cpu_algo;
	int		bind_cpu_ht;

	proxy_stat_t	stat;
	pthread_mutex_t	lock;
	int		stop;
	pthread_t	main_tid;
} proxy_t;

typedef struct	proxy_arg {
	policy_t	policies[MAX_POLICY];/* policies in proxy */
	int		npolicy;

	int		nworker;
	int		max;
	int		maxfd;
	int		dbglvl;	

	int		use_splice;
	int		use_nb_splice;

	int		bind_cpu;
	int		bind_cpu_algo;
	int		bind_cpu_ht;

} proxy_arg_t;

extern	proxy_t		g_proxy;

extern int 
proxy_main(proxy_arg_t *arg);

#define	PROXY_LOCK()	pthread_mutex_lock(&g_proxy.lock)
#define	PROXY_UNLOCK()	pthread_mutex_unlock(&g_proxy.lock)
//#define	PROXY_LOCK()	{}
//#define	PROXY_UNLOCK()		{}

#endif	/* end of FZ_PROXY_H */


