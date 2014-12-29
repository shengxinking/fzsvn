/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include "policy.h"
#include "proxy_debug.h"

policy_t * 
policy_alloc(void)
{
	policy_t *pl;

	pl = calloc(1, sizeof(*pl));
	if (!pl)
		ERR_RET(NULL, "malloc memory for policy_t failed\n");

	CBLIST_INIT(&pl->list);

	return pl;
}

int 
policy_free(policy_t *pl)
{
	int refcnt;

	if (!pl)
		ERR_RET(-1, "invalid argument\n");

	refcnt = __sync_fetch_and_sub(&pl->refcnt, 1);

	if (refcnt == 0) {

		if (pl->cfg.listener)
			listener_free(pl->cfg.listener);
		if (pl->cfg.svrpool)
			svrpool_free(pl->cfg.svrpool);

		policy_free_data(pl);

		free(pl);
	}

	return 0;
}

policy_t * 
policy_clone(policy_t *pl)
{
	if (!pl)
		ERR_RET(NULL, "invalid argument\n");

	__sync_fetch_and_add(&pl->refcnt, 1);

	return pl;
}

int 
policy_init_data(policy_t *pl)
{
	if (!pl)
		ERR_RET(-1, "invalid argument\n");

	if (!pl->cfg.listener || !pl->cfg.svrpool)
		ERR_RET(-1, "policy didn't have listener or svrpool\n");

	pl->data.spdata = svrpool_alloc_data(pl->cfg.svrpool);
	if (!pl->data.spdata)
		ERR_RET(-1, "alloc svrpool data failed\n");

	pl->data.ltndata = listener_alloc_data(pl->cfg.listener);
	if (!pl->data.ltndata)
		ERR_RET(-1, "alloc listener data failed\n");

	return 0;
}

int 
policy_free_data(policy_t *pl)
{
	if (!pl)
		ERR_RET(-1, "invalid argument\n");

	if (pl->data.spdata) {
		svrpool_free_data(pl->data.spdata);
		pl->data.spdata = NULL;
	}

	if (pl->data.ltndata) {
		listener_free_data(pl->data.ltndata);
		pl->data.ltndata = NULL;
	}

	return 0;
}

svrpool_data_t *
policy_clone_spdata(policy_t *pl)
{
	svrpool_data_t *spdata;

	if (!pl)
		ERR_RET(NULL, "invalid argument\n");

	/* need read lock for protect pointer */

	if (!pl->data.spdata)
		ERR_RET(NULL, "not svrpool data\n");

	spdata = svrpool_clone_data(pl->data.spdata);

	return spdata;
}

listener_data_t * 
policy_clone_ltndata(policy_t *pl)
{
	listener_data_t *ltndata;

	if (!pl)
		ERR_RET(NULL, "invalid argument\n");

	/* need read lock for protect pointer */

	if (!pl->data.ltndata)
		ERR_RET(NULL, "not listener data\n");

	ltndata = listener_clone_data(pl->data.ltndata);

	return ltndata;
}

int 
policy_print(const policy_t *pl, const char *prefix)
{
	const policy_cfg_t *plcfg;

	if (!pl || !prefix)
		ERR_RET(-1, "invalid argument\n");
	
	plcfg = &pl->cfg;

	printf("%spolicy(%p) <%s>:\n", prefix, pl, plcfg->name);
	printf("%s\tmode:           (%d)\n", prefix, plcfg->mode);
	printf("%s\tlistener:       (%p)\n", prefix, plcfg->listener);
	printf("%s\tserver_pool:    (%p)\n", prefix, plcfg->svrpool);

	return 0;
}

