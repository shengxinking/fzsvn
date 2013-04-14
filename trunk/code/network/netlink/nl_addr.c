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

#include "netutil.h"
#include "netlink.h"
#include "libnetlink.h"

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
	char buf[RTNL_BUF_LEN];
	int group;
	int ret;
	size_t alen;
	u_int32_t ip4, brd;

	if (!addr)
		return -1;

	if (family == AF_INET) {
		group = RTMGRP_IPV4_IFADDR;
		alen = 4;
	}
	else if (family == AF_INET6) {
		group = RTMGRP_IPV6_IFADDR;
		alen = 16;
	}
	else {
		_NL_ADDR_ERR("invalid family %d\n", family);
		return -1;
	}

	if (rtnl_open(&rtx, group))
		return -1;
	
	memset(buf, 0, sizeof(buf));
	
	/* set nlmsghdr */
	nlh = (struct nlmsghdr *)buf;
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*req));
	nlh->nlmsg_type = RTM_NEWADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_REPLACE;
	nlh->nlmsg_seq = rtx.seq;
	nlh->nlmsg_pid = rtx.local.nl_pid;

	/* set addr request */
	req = NLMSG_DATA(nlh);
	req->ifa_family = family;
	req->ifa_prefixlen = cidr;
	req->ifa_scope = 0;
	req->ifa_flags = IFA_F_PERMANENT;
	req->ifa_index = 1;
	printf("nlhmsg_len %d\n", nlh->nlmsg_len);
	
	/* add addr */
	if (rtnl_add_attr(nlh, sizeof(buf), IFA_LOCAL, addr, alen)) {
		_NL_ADDR_ERR("add IFA_LOCAL failed\n");
		rtnl_close(&rtx);
		return -1;
	}
	printf("nlhmsg_len %d\n", nlh->nlmsg_len);
	
	/* add broad cast only for IPv4 */
	if (family == AF_INET) {
		ip4 = *((u_int32_t *)addr);
		brd = ip4 | ~(ip4_cidr_to_mask(cidr));
		printf("ip4 is %u.%u.%u.%u\n", IP4_QUAD(ip4));
		printf("brd is %u.%u.%u.%u\n", IP4_QUAD(brd));
		if (rtnl_add_attr(nlh, sizeof(buf), IFA_BROADCAST, &brd, alen)) {
			_NL_ADDR_ERR("add IFA_BROADCAST failed\n");
			rtnl_close(&rtx);
			return -1;
		}
		printf("nlhmsg_len %d\n", nlh->nlmsg_len);
	}
	
	/* send request and check reply */
	ret = rtnl_talk(&rtx, nlh);
	rtnl_close(&rtx);

	return ret;
}

int 
nl_addr_del(int index, int family, void *addr, int cidr)
{
	_nl_addr_flush(NULL, NULL);
	return 0;
}

int 
nl_addr_flush(int index, int family)
{
	_nl_addr_flush(NULL, NULL);
	return 0;
}

int 
nl_addr_find(int index, int family, void *addr)
{
	_nl_addr_list(NULL, NULL);
	return 0;
}

int 
nl_addr_list(int index, int family, nl_print print, void *arg)
{
	_nl_addr_list(NULL, NULL);
	return 0;
}
