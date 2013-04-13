/**
 *	@file	libnetlink.h
 *
 *	@brief	The netlink basic APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_LIBNETLINK_H
#define FZ_LIBNETLINK_H

#include <linux/rtnetlink.h>
#include <sys/types.h>

#define	RTNL_DUMP_LEN	8192
#define	RTNL_BUF_LEN	1024

typedef struct rtnl_ctx {
	int		fd;		/* socket fd */
	struct sockaddr_nl local;	/* local netlink address */
	struct sockaddr_nl peer;	/* peer netlink address(kernel) */
	u_int32_t	seq;		/* sequence number */
	u_int32_t	dump;		/* dump sequence number */
} rtnl_ctx_t;

typedef	int	(* rtnl_filter)(struct nlmsghdr *msg, void *arg);
typedef int	(* rtnl_print)(unsigned long *parg);

extern int 
rtnl_open(rtnl_ctx_t *rtx, int group);

extern int 
rtnl_close(rtnl_ctx_t *rtx);

extern int 
rtnl_send(rtnl_ctx_t *rtx, struct nlmsghdr *msg);

extern int 
rtnl_recv(rtnl_ctx_t *rtx, char *buf, size_t len);

extern int 
rtnl_talk(rtnl_ctx_t *rtx, struct nlmsghdr *msg);

extern int 
rtnl_dump_request(rtnl_ctx_t *rtx, int family, int type);

extern int 
rtnl_dump_filter(rtnl_ctx_t *rtx, rtnl_filter filter, void *arg);

extern int 
rtnl_add_attr(struct nlmsghdr *nlh, size_t len, int type, void *data, size_t dlen);

extern int 
rtnl_parse_attr(struct rtattr *rtb[], int nrtb, struct rtattr *rta, size_t len);

extern int 
rtnl_get_error(int err);

#endif /* end of FZ_LIBNETLINK_H  */


