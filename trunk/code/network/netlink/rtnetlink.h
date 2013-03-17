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

typedef struct rtnl_ctx {
	int		fd;	/* socket fd */
	struct sockaddr_nl local;/* local netlink address */
	struct sockaddr_nl peer;/* peer netlink address(kernel) */
} rtnl_ctx;

extern int 
rtnl_open(void);

extern int 
rtnl_send(int fd, struct nlmsghdr *msg);

extern struct nlmsghdr * 
rtnl_recv(int fd);

extern int 
rtnl_error(int err);


#endif /* end of FZ_  */


