/**
 *	@file	policy.h
 *
 *	@brief	policy structure and APIs
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_POLICY_H
#define FZ_POLICY_H

#include <sys/param.h>

#include "cblist.h"
#include "svrpool.h"
#include "listener.h"
#include "proxy_common.h"

enum {
	PL_MODE_REVERSE,
	PL_MODE_TRAPT,
	PL_MODE_TPROXY,
};

/**
 *	Policy config.
 */
typedef struct policy_cfg {
	char		name[MAX_NAME];
	int		mode;		/* run mode */
	listener_t	*listener;	/* listener */
	svrpool_t	*svrpool;	/* svrpool */
} policy_cfg_t;

/**
 *	Policy running data
 */
typedef struct policy_data {
	listener_data_t	*ltndata;	/* listener running data */
	svrpool_data_t	*spdata;	/* svrpool running data */
} policy_data_t;

/**
 *	Policy statistic data
 */
typedef struct policy_stat {
	u_int64_t	nconn;		/* number of connection */
} policy_stat_t;

/**
 *	Policy structure.
 */
typedef struct policy {
	policy_cfg_t	cfg;		/* config */
	policy_data_t	data;		/* running data */
	policy_stat_t	stat;		/* statistic data */

	int		refcnt;		/* reference count */

	cblist_t	list;		/* list in proxy_t->policy */
} policy_t;

/**
 *	Alloc a new policy and return it.
 *
 *	Return pointer if success, NULL on error.
 */
extern policy_t *  
policy_alloc(void);

/**
 *	Clone a policy @pl, incement @refcnt avoid 
 *	@pl pointer invalid.
 *
 *	Return @pl if success, NULL on error.
 */
extern policy_t * 
policy_clone(policy_t *pl);

/**
 *	Free policy @pl alloced by @policy_alloc until 
 *	@refcnt is zero. It also free running data alloced 
 *	by @policy_init_data and free @listern/@svrpool in config.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
policy_free(policy_t *pl);

/**
 *	Print policy @pl's config. the prefix string 
 *	is @prefix
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
policy_print(const policy_t *pl, const char *prefix);

/**
 *	Init policy @pl running data.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
policy_init_data(policy_t *pl);

/**
 *	Free policy @pl running data alloced 
 *	by @policy_init_data.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
policy_free_data(policy_t *pl);

/**
 *	Clone policy @pl's svrpool running data.
 *
 *	Return pointer if success, NULL on error.
 */
extern svrpool_data_t * 
policy_clone_spdata(policy_t *pl);

/**
 *	Clone policy @pl's listener running data.
 *
 *	Return pointer if success, NULL on error.
 */
extern listener_data_t * 
policy_clone_ltndata(policy_t *pl);


#endif /* end of FZ_POLICY_H */


