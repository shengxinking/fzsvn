/**
 *	@file	rtnetlink.h
 *
 *	@brief	The NETLINK_ROUTE netlink APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_RTNETLINK_H
#define FZ_RTNETLINK_H

#include <linux/rtnetlink.h>

#define	RTNL_DUMP_LEN	8192

typedef struct rtnl_ctx {
	int		fd;		/* socket fd */
	struct sockaddr_nl local;	/* local netlink address */
	struct sockaddr_nl peer;	/* peer netlink address(kernel) */
	u_int32_t	seq;		/* sequence number */
	u_int32_t	dump_seq;	/* dump sequence number */
} rtnl_ctx_t;

typedef	int	(*rtnl_filter)(struct nlmsg *msg, void *arg);
typedef void	(*rtnl_print)(unsigned long *parg);

extern int 
rtnl_open(rtnl_ctx_t *rtx);

extern int 
rtnl_send(rtnl_ctx_t *rtx, struct nlmsghdr *msg);

extern struct nlmsghdr * 
rtnl_recv(rtnl_ctx_t *rtx);

extern int 
rtnl_send_request(rtnl_ctx_t *rtx, int family, int type);

extern int 
rtnl_dump_request(rtnl_ctx_t *rtx, int family, int type);

extern int 
rtnl_dump_filter(rtnl_ctx_t *rtx, int family, int cmd);

extern int 
rtnl_add_attr(struct nlmsg *msg, int type, void *data, size_t len);

extern int 
rtnl_parse_attr(struct nlmsg *msg, struct rtattr *rtattrs, int nrtattr);

extern int 
rtnl_get_error(int err);

#endif /* end of FZ_RTNETLINK_H  */


