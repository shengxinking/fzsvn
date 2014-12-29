/**
 *	@file	server_pool.h
 *
 *	@brief	Real server/server-pool structure and APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_SVRPOOL_H
#define FZ_SVRPOOL_H

#include <sys/param.h>

#include "cblist.h"
#include "ip_addr.h"
#include "ssl_util.h"
#include "certset.h"
#include "proxy_common.h"

/**
 *	svrpool loadbalance algorithm.
 */
typedef enum svrpool_algo{
	SP_ALGO_RR,
	SP_ALGO_WRR,
	SP_ALGO_LC,
	SP_ALGO_HASH,
	SP_ALGO_MAX,
} svrpool_algo_e;

/**
 *	physical server config.
 */
typedef struct server_cfg {
	ip_port_t	address;	/* server address */
	int		weight;		/* weight for WRR algorithm */
	int		ssl;		/* ssl enabled or not */
	certset_t	*cert;		/* client certificate */
} server_cfg_t;

/**
 *	Physical server statistic data.
 */
typedef struct server_stat {
	u_int32_t	nconn;		/* number of connections */
	u_int32_t	totconn;	/* total connections */
} server_stat_t;

/**
 *	Physical server structure.
 */
typedef struct server {
	server_cfg_t	cfg;		/* config */
	server_stat_t	stat;		/* statistic data */
	int		refcnt;		/* reference count */
	cblist_t	list;		/* list into svrpool's @svrlist */
} server_t;

/**
 *	Physical server running data, used in @svrpool_data.
 */
typedef struct server_data {
	server_t	*server;	/* server it belong */
	ssl_ctx_t	*sslctx;	/* SSL context */
	cblist_t	ssnlist;	/* sessions in this server */
	server_stat_t	stat;		/* statistic data */
	int		refcnt;		/* reference count */
} server_data_t;

/**
 *	Server-Pool config.
 */
typedef struct svrpool_cfg {
	char		name[MAX_NAME];	/* name */
	svrpool_algo_e	algo;		/* load balance algorithm */
} svrpool_cfg_t;

/**
 *	Server-Pool statistic data.
 */
typedef struct svrpool_stat {
	u_int32_t	nconn;		/* number of connection */
	u_int32_t	totconn;	/* total connections */
} svrpool_stat_t;

/**
 *	Server-Pool structure.
 */
typedef struct svrpool {
	svrpool_cfg_t	cfg;		/* config */
	svrpool_stat_t	stat;		/* statistic data */
	cblist_t	svrlist;	/* server list */
	int		nserver;	/* number of server in list */
	int		refcnt;		/* reference count */
	cblist_t	list;		/* list into proxy's @splist */
} svrpool_t;

/**
 *	Server-pool running data, using in @policy_data
 */
typedef struct svrpool_data {
	server_data_t	*servers[MAX_SERVER];/* server running data array */
	int		nserver;	/* number of server_data */
	u_int32_t	gcd;		/* gcd value for WRR algorithm */
	int		rrpos;		/* rr pos */
	u_int32_t	wrrpos;		/* wrr pos */
	int		refcnt;		/* reference count */
} svrpool_data_t;

/**
 *	Alloc a new server and return it.
 *
 *	Return pointer if success, NULL on error.
 */
extern server_t * 
server_alloc(void);

/**
 *	Clone a server @svr and increment @refcnt.
 *
 *	Return @svr if success, NULL on error.
 */
extern server_t * 
server_clone(server_t *svr);

/**
 *	Free server @svr until @refcnt is zero, 
 *	it'll decrement @refcnt.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
server_free(server_t *svr);

/**
 *	Alloc a new server_data and return it.
 *
 *	Return pointer if success, NULL on error.
 */
extern server_data_t * 
server_alloc_data(server_t *svr);

/**
 *	Clone a server_data @svrdata and increment @refcnt.
 *
 *	Return @svrdata if success, NULL on error.
 */
extern server_data_t * 
server_clone_data(server_data_t *svrdata);

/**
 *	Free server_data @svrdata until @refcnt is zero, 
 *	it'll decrement @refcnt.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
server_free_data(server_data_t *svrdata);

/**
 *	Alloc a new svrpool and return it.
 *
 *	Return pointer if success, NULL on error.
 */
extern svrpool_t * 
svrpool_alloc(void);

/**
 *	Clone svrpool @sp and incement @refcnt.
 *
 *	Return @sp if success, NULL on error.
 */
extern svrpool_t * 
svrpool_clone(svrpool_t *sp);

/**
 *	Free svrpool @sp until @refcnt is zero, 
 *	it'll decrement @refcnt.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
svrpool_free(svrpool_t *sp);

/**
 *	Print svrpool @sp config, output prefix is @prefix.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
svrpool_print(const svrpool_t *sp, const char *prefix); 

/**
 *	Alloc svrpool data for svrpool @sp and
 *	return it.
 *
 *	Return pointer if success, NULL on error.
 */
extern svrpool_data_t * 
svrpool_alloc_data(svrpool_t *sp);

/**
 *	Clone svrpool data @spdata, it'll increment @refcnt.
 *
 *	Return @spdata if success, NULL on error.
 */
extern svrpool_data_t * 
svrpool_clone_data(svrpool_data_t *spdata);

/**
 *	Free svrpool data @spdata until @refcnt is zero,
 *	it'll decrement @refcnt.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
svrpool_free_data(svrpool_data_t *spdata);

/**
 *	Get a server_data from svrpool data @spdata 
 *	according svrpool loadbalance algorithm.
 *	It'll clone server_data in @spdata.
 *
 *	Return pointer if success, NULL on error.
 */
extern server_data_t *
svrpool_get_rp_server(svrpool_data_t *spdata);

/**
 *	Get a server_data from svrpool data @spdata 
 *	according server address @svraddr.
 *	It'll clone server_data in @spdata.
 *
 *	Return pointer if success, NULL on error.
 */
extern server_data_t * 
svrpool_get_tp_server(svrpool_data_t *spdata, ip_port_t *svraddr);

#endif /* end of FZ_SVRPOOL_H */

