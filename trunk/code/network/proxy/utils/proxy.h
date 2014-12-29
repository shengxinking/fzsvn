/**
 *	@file	proxy.h
 *
 *	@brief	proxy structure and APIs
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PROXY_H
#define FZ_PROXY_H

#include "cblist.h"
#include "thread.h"
#include "svrpool.h"
#include "policy.h"
#include "proxy_common.h"

/**
 *	Proxy config.
 */
typedef struct proxy_cfg {
	int		nworker;	/* number of worker thread */
	int		nice;		/* worker thread priority */
	int		naccept;	/* number of accept in one time */
	int		use_splice;	/* using splice */
	int		use_nbsplice;	/* using nbsplice */
	int		bind_cpu;	/* enable bind cpu */
	int		bind_cpu_algo;	/* bind cpu algo: rr | odd | even */
	int		bind_cpu_ht;	/* bind cpu HT: full | low | high */
	int		maxconn;	/* max connection in proxy */
	int		debug;		/* debug level */
	int		flow;		/* flow debug level */
	int		http;		/* http debug level */
	int		timestamp;	/* timestamp for debug output */
} proxy_cfg_t;

/**
 *	Proxy running data.
 */
typedef struct proxy_data {
	thread_t	workers[MAX_WORKER];/* worker thread */
	thread_t	status;		/* status thread */
	int		nbsplice_fd;	/* nb_splice fd */
	int		maxfd;		/* max fd */
} proxy_data_t;

/**
 *	Proxy statistic data.
 */
typedef struct proxy_stat {
	u_int64_t	nlive;		/* live connection */
	u_int64_t	nhttplive;	/* live http connection */
	u_int64_t	nhttpslive;	/* live https connection */
} proxy_stat_t;

/**
 *	Proxy structure.
 */
typedef struct proxy {
	proxy_cfg_t	cfg;		/* config */
	proxy_data_t	data;		/* running data */
	proxy_stat_t	stat;		/* statistic data */
	
	cblist_t	certlist;	/* client certificate list */
	int		ncert;

	cblist_t	ltnlist;	/* listener list */
	int		nlistener;	/* number of listener */
	
	cblist_t	splist;		/* svrpool list */
	int		nsvrpool;	/* number of svrpool */

	cblist_t	pllist;		/* policy list */
	int		npolicy;	/* number of policy */
} proxy_t;

extern volatile int	g_stop;		/* stop flags: 1 proxy need stopped. */
extern pthread_t	g_maintid;	/* main thread tid */

/**
 *	Alloc a new proxy and return it.
 *
 *	Return proxy object if success, NULL on error.
 */
extern proxy_t *  
proxy_alloc(void);

/**
 *	Free a proxy alloced by @proxy_alloc(), it'll free all
 *	listeners/svrpools/policies added by @proxy_add_xxx().
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proxy_free(proxy_t *py);

/**
 *	Print proxy config. Include all listeners/svrpools/policy 
 *	config 
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proxy_print(const proxy_t *py);

/**
 *	Add certset @cert into proxy @py
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proxy_add_certset(proxy_t *py, certset_t *cert);

/**
 *	Add listener @ltn into proxy @py
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proxy_add_listener(proxy_t *py, listener_t *ltn);

/**
 *	Add svrpool @sp into proxy @py
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proxy_add_svrpool(proxy_t *py, svrpool_t *sp);

/**
 *	Add policy @pl into proxy @py
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proxy_add_policy(proxy_t *py, policy_t *pl);

/**
 *	Find certset which name is @name in proxy @py.
 *
 *	Return pointer if find, NULL on error or not found.
 */
certset_t *
proxy_find_certset(proxy_t *py, const char *name);

/**
 *	Find listener which name is @name in proxy @py.
 *
 *	Return pointer if find, NULL on error or not found.
 */
listener_t *
proxy_find_listener(proxy_t *py, const char *name);

/**
 *	Find svrpool which name is @name in proxy @py.
 *
 *	Return pointer if find, NULL on error or not found.
 */
svrpool_t * 
proxy_find_svrpool(proxy_t *py, const char *name);

/**
 *	Find policy which name is @name in proxy @py.
 *
 *	Return pointer if find, NULL on error or not found.
 */
policy_t * 
proxy_find_policy(proxy_t *py, const char *name);

/**
 *	The main function of proxy.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proxy_main(proxy_t *py);


#endif /* end of FZ_PROXY_H */


