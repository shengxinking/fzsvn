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

#include "trapt_util.h"
#include "proxy_debug.h"
#include "proxy.h"

int 
tat_add_policy(const tat_policy_t *pl)
{
	int fd;
	int ret;

	if (!pl) 
		ERR_RET(-1, "invalid argument\n");

	fd = open(TRAPT_DEVNAME, O_RDONLY);
	if (fd < 0)
		ERR_RET(-1, "open %s failed: %s\n", TRAPT_DEVNAME, ERRSTR);

	ret = ioctl(fd, TRAPT_IOC_ADD_POLICY, pl);
	if (ret) 
		ERR("ioctl %s failed: %s\n", TRAPT_DEVNAME, ERRSTR);

	close(fd);
	return ret;
}

int 
tat_get_policy(const tat_addr_t *vsaddr, tat_policy_t *pl)
{
	return 0;
}

int 
tat_del_policy(const tat_addr_t *vsaddr)
{
	int fd;
	int ret;
	tat_policy_t pl;
	
	if (vsaddr == 0)
		ERR_RET(-1, "invalid argument\n");

	fd = open(TRAPT_DEVNAME, O_RDONLY);
	if (fd < 0)
		ERR_RET(-1, "open %s failed: %s\n", TRAPT_DEVNAME, ERRSTR);

	memset(&pl, 0, sizeof(pl));
	pl.vsaddr = vsaddr->addr;
	pl.vsport = vsaddr->port;

	ret = ioctl(fd, TRAPT_IOC_DEL_POLICY, &pl);
	if (ret) 
		ERR("ioctl %s failed: %s\n", TRAPT_DEVNAME, ERRSTR);

	close(fd);
	return ret;
}

int 
tat_flush_policies(void)
{
	int fd;
	int ret;
	tat_policy_t pl;

	fd = open(TRAPT_DEVNAME, O_RDONLY);
	if (fd < 0)
		ERR_RET(-1, "open %s failed: %s\n", TRAPT_DEVNAME, ERRSTR);

	memset(&pl, 0, sizeof(pl));
	
	ret = ioctl(fd, TRAPT_IOC_FLUSH_POLICY, &pl);
	if (ret) 
		ERR("ioctl %s failed: %s\n", TRAPT_DEVNAME, ERRSTR);

	close(fd);
	return ret;
}

int 
tat_add_policies(proxy_t *py)
{
	size_t len;
	policy_t *pl;
	server_t *svr;
	svrpool_t *sp;
	listener_t *lt;
	tat_policy_t *tpl;
	tat_addr_t *psaddr;
	
	len = sizeof(*tpl) + MAX_SERVER * sizeof(tat_addr_t);
	printf("length is %lu\n", len);
	tpl = malloc(len);
	if (!tpl)
		ERR_RET(-1, "malloc memory failed: %s\n", ERRSTR);

	CBLIST_FOR_EACH(&py->pllist, pl, list) {

		if (pl->cfg.mode != PL_MODE_TRAPT)
			continue;

		sp = pl->cfg.svrpool;
		lt = pl->cfg.listener;

		memset(tpl, 0, len);
		strcpy(tpl->name, pl->cfg.name);
		tpl->vsaddr = lt->cfg.address._addr4.s_addr;
		tpl->vsport = lt->cfg.address.port;
		
		psaddr = tpl->psaddrs;
		CBLIST_FOR_EACH(&sp->svrlist, svr, list) {
			psaddr->addr = svr->cfg.address._addr4.s_addr;
			psaddr->port = svr->cfg.address.port;
			psaddr += sizeof(*psaddr);
		}
		tpl->npsaddr = sp->nserver;

		DBG(1, "add trapt policy %s to kernel\n", pl->cfg.name);

		if (tat_add_policy(tpl))
			break;
	}

	printf("111111111\n");

	free(tpl);

	return 0;
}

int 
tat_get_dstaddr(int fd, tat_addr_t *addr)
{
	socklen_t len;
	struct sockaddr_in ip;

	if (fd < 0 || !addr)
		ERR_RET(-1, "invalid argument\n");

	len = sizeof(ip);
	memset(&ip, 0, len);
	if (getsockopt(fd, IPPROTO_IP, TRAPT_SO_GET_DST, &ip, &len)) 
		ERR_RET(-1, "getsockopt failed: %s\n", ERRSTR);

	addr->addr = ip.sin_addr.s_addr;
	addr->port = ip.sin_port;

	return 0;
}

int 
tat_set_dstaddr(int fd, const tat_addr_t *addr)
{
	socklen_t len;

	if (fd < 0 || !addr)
		return -1;

	len = sizeof(*addr);
	if (setsockopt(fd, IPPROTO_IP, TRAPT_SO_SET_DST, addr, len))
		ERR_RET(-1, "setsockopt failed: %s\n", ERRSTR);

	return 0;

}

