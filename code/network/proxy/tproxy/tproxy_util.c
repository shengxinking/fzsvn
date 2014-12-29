/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <fcntl.h>

#include "cblist.h"
#include "tproxy_util.h"
#include "proxy_debug.h"
#include "proxy.h"

int 
tp_add_policy(const tp_policy_t *pl)
{
	return 0;
}

int 
tp_get_policy(const ip_port_t *vsaddr, tp_policy_t *pl)
{
	return 0;
}

int 
tp_del_policy(const ip_port_t *vsaddr)
{
	return 0;
}

int 
tp_flush_policies(void)
{
	int fd;
	int ret;

	fd = open(TPROXY_DEVNAME, O_RDONLY);
	if (fd < 0) 
		ERR_RET(-1, "open %s failed: %s\n", TPROXY_DEVNAME, ERRSTR);

	ret = ioctl(fd, TPROXY_IOC_FLUSH_POLICIES, 0);
	if (ret) 
		ERR("ioctl %s failed: %s\n", TPROXY_DEVNAME, ERRSTR);

	close(fd);
	return ret;
}

int
tp_add_policies(proxy_t *py)
{
	int fd;
	int ret;
	size_t len;
	int npl;
	policy_t *pl;
	server_t *svr;
	svrpool_t *sp;
	listener_t *lt;
	tp_policy_t *tpl;
	tp_policies_t *tpls;
	ip_port_t *psaddr;

	if (!py)
		ERR_RET(-1, "invalid argument\n");

	if (py->npolicy < 1)
		return -1;

	len = sizeof(tp_policy_t) + MAX_SERVER * sizeof(ip_port_t);
	tpls = calloc(1, len * py->npolicy);
	if (!tpls)
		ERR_RET(-1, "calloc memory failed: %s\n", ERRSTR);

	npl = 0;
	len = 0;
	tpl = tpls->policies;
	CBLIST_FOR_EACH(&py->pllist, pl, list) {

		/* not tproxy policy */
		if (pl->cfg.mode != PL_MODE_TPROXY)
			continue;

		sp = pl->cfg.svrpool;
		lt = pl->cfg.listener;

		strcpy(tpl->name, pl->cfg.name);
		tpl->vsaddr = lt->cfg.address;
		tpl->npsaddr = sp->nserver;
		
		psaddr = tpl->psaddrs;
		CBLIST_FOR_EACH(&sp->svrlist, svr, list) {
			*psaddr = svr->cfg.address;
			psaddr += sizeof(*psaddr);
		}

		DBG(1, "add tproxy tpolicy %s to kernel\n", pl->cfg.name);

		tpl = (tp_policy_t *)psaddr;
		npl++;
	}

	/* not policies */
	if (npl <= 0) {
		free(tpls);
		return 0;
	}

	tpls->size = (unsigned char *)tpl - (unsigned char *)tpls;

	fd = open(TPROXY_DEVNAME, O_RDONLY);
	if (fd < 0)  {
		free(tpls);
		ERR_RET(-1, "open %s failed: %s\n", TPROXY_DEVNAME, ERRSTR);
	}

	ret = ioctl(fd, TPROXY_IOC_SET_POLICIES, tpls);
	if (ret) 
		ERR("ioctl %s failed: %s\n", TPROXY_DEVNAME, ERRSTR);

	close(fd);
	free(tpls);
	return ret;
}



