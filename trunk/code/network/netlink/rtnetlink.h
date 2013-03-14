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

extern int 
rtnl_open(void);

extern int 
rtnl_send(int fd, struct nlmsghdr *msg);

extern struct nlmsghdr * 
rtnl_recv(int fd);

extern int 
rtnl_error(int err);


#endif /* end of FZ_  */


