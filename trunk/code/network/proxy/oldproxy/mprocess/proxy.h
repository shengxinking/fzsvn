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
#include "fd_epoll.h"
#include "gcc_common.h"

#define PROXY_TIMEOUT		5
#define PROXY_PKTSIZE		4096
#define	MAX_REALSVR		128
#define	MAX_POLICY		32

typedef struct proxy_stat {
	/* the session information */
	u_int64_t       nhttp;          /* total http session number */
	u_int64_t       nhttps;         /* total https session number */
	u_int64_t       naccept;        /* total session number (http+https) */
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

struct proxy2;

typedef struct policy {
	struct proxy2	*proxy;		/* the proxy of policy */
	int		index;		/* the policy index */
	int		httpfd;		/* the HTTP listen fd */
	int		httpsfd;	/* the HTTPS listen fd */
	ip_port_t	httpaddr;	/* HTTP server address */
	ip_port_t	httpsaddr;	/* HTTPS server address */
	ip_port_t	rsaddrs[MAX_REALSVR];/* real server address */
	int		nrsaddr;	/* number of realserver */
	int		pos;		/* the RR pos for loadbalance */
} policy_t;

typedef struct proxy {
	int		send_epfd;	/* send epoll fd */
	int		recv_epfd;	/* recv epoll fd */
	int		epfd;		/* the send/recv epoll */
	int		nepollevt;	/* the epoll event number */
	policy_t	policies[MAX_POLICY];/* the policies in proxyd */
	int		npolicy;	/* number of policy */
	struct epoll_event *events;	/* used for epoll_wait() */
	objpool_t	*pktpool;	/* memory pool for packet */
	objpool_t	*ssnpool;	/* memory pool for session */
	session_table_t	*sessions;	/* session table */
	int		nblocked;	/* number of send blocked socket */
	int		max;		/* max live sessions in proxy */ 

	int		use_splice;	/* use splice */
	int		use_nb_splice;	/* use nb_splice */
	int		splice_fd;	/* nb_splice fd */

	int		bind_cpu;
	int		bind_cpu_algo;
	int		bind_cpu_ht;

	proxy_stat_t	stat;		/* current connections in proxy */
} proxy_t;

typedef struct proxy2 {
	policy_t	policies[MAX_POLICY];/* the policies in proxyd */
	int		npolicy;	/* number of policy */

	fd_epoll_t	*fe;		/* the fd epoll object */
	objpool_t	*pktpool;	/* memory pool for packet */
	objpool_t	*ssnpool;	/* memory pool for session */

	int		max;
	int		use_splice;	/* use splice */
	int		use_nb_splice;	/* use nb_splice */
	int		splice_fd;	/* nb_splice fd */

	int		bind_cpu;
	int		bind_cpu_algo;
	int		bind_cpu_ht;

	proxy_stat_t	stat;		/* current connections in proxy */
} proxy2_t;

/**
 *	The proxy args.
 */
typedef struct proxy_arg {
	int		index;		/* the child proxy index */
	policy_t	policies[MAX_POLICY];/* the policies */
	int		npolicy;	/* number of policy */
	ip_port_t	svraddr;	/* server address */
	ip_port_t	rsaddrs[MAX_REALSVR];/* real server */
	int		nrsaddr;	/* number of real server */
	int		dbglvl;		/* the debug level */
	int		max;		/* max live session in proxy */
	int		use_splice;	/* use system splice */
	int		use_nb_splice;	/* use nb_splice */
	int		bind_cpu;
	int		bind_cpu_algo;
	int		bind_cpu_ht;
} proxy_arg_t;

/**
 *	the main entry of proxy, it listen on @ip:@port, and the real server
 *	is @rip:@rport.
 *
 *	return 0 if success, -1 on error.
 */
extern 
int proxy_main(proxy_arg_t *arg);


#endif /* end of FZ_PROXY_H */

