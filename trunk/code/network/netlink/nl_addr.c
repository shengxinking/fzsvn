/**
 *	@file	nl_addr.c
 *
 *	@brief	netlink address APIs.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2013-04-02
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_addr.h>

#include "netlink.h"
#include "rtnetlink.h"

#define	_NL_ADDR_ERR(fmt, args...)	\
	printf("%s:%d: "fmt, __FILE__, __LINE__, ##args)

static int 
_nl_addr_flush(struct nlmsghdr *nlh, void *arg)
{
	return 0;
}


static int 
_nl_addr_list(struct nlmsghdr *nlh, void *arg)
{
	return 0;
}


int 
nl_addr_add(int index, int family, void *addr, int cidr)
{
	rtnl_ctx_t rtx;
	struct nlmsghdr *nlh;
	struct ifaddrmsg *req;
	char buf[RTNL_REQ_LEN];
	size_t alen = 4;

	if (!addr)
		return -1;

	if (family != AF_INET && family != AF_INET6) {
		_NL_ADDR_ERR("invalid family\n");
		return -1;
	}

	if (rtnl_open(&rtx))
		return -1;
	
	memset(buf, 0, sizeof(buf));
	
	/* set nlmsghdr */
	nlh = (struct nlmsghdr *)buf;
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*req));
	nlh->nlmsg_type = RTM_NEWADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_APPEND;
	nlh->nlmsg_seq = rtx.seq;
	nlh->nlmsg_pid = rtx.local.nl_pid;

	/* set addr request */
	req = NLMSG_DATA(nlh);
	req->ifa_family = family;
	req->ifa_prefixlen = cidr;
	req->ifa_flags = IFA_F_PERMANENT;
	req->ifa_scope = 0;
	req->ifa_index = index;
	
	/* add addr */
	if (family == AF_INET6) alen = 16;
	if (rtnl_add_attr(nlh, sizeof (buf), IFA_LOCAL, addr, alen)) {
		_NL_ADDR_ERR("add IFA_LOCAL failed\n");
		return -1;
	}
	
	/* send request */
	return rtnl_send_request(&rtx, nlh);
}

int 
nl_addr_del(int index, int family, void *addr, int cidr)
{
	return 0;
}

int 
nl_addr_flush(int index, int family)
{
	return 0;
}

int 
nl_addr_find(int index, int family, void *addr)
{
	return 0;
}

int 
nl_addr_list(int index, int family, nl_print print, void *arg)
{
	return 0;
}
