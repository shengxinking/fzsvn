/*
 *  add a route rule into kernel route table
 *
 *  write by Forrest.zhang
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libnetlink.h"
#include "route.h"

static void usage(void)
{
    printf("route add <destination> <gateway> <device>\n");
}

int add_route(int argc, char* argv[])
{
    __u32                  dest;
    __u32                  gateway;
    struct rtnl_handle     rth;
    struct {
	struct nlmsghdr    n;
	struct rtmsg       r;
	char               buf[1024];
    } req;
   
    if (argc != 5) {
	usage();
	return -1;
    }

    dest = ip_u32(argv[2]);
    gateway = ip_u32(argv[3]);

    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;
    req.n.nlmsg_type = RTM_NEWROUTE;

    req.r.rtm_family = AF_INET;
    req.r.rtm_table = RT_TABLE_MAIN;
    req.r.rtm_protocol = RTPROT_BOOT;
    req.r.rtm_scope = RT_SCOPE_UNIVERSE;
    req.r.rtm_type = RTN_UNICAST;

    add_rtattr(&req.n, sizeof(req), RTA_DST, &dest, sizeof(dest));
    add_rtattr(&req.n, sizeof(req), RTA_GATEWAY, &gateway, sizeof(gateway));

    if (rtnl_open(&rth))
	return -1;

    if (rtnl_send(&rth, &req.n) < 0)
	return -1;

    return 0;
}
