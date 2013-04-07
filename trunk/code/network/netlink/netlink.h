/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_NETLINK_H
#define FZ_NETLINK_H

#include <linux/netlink.h>

typedef	int	(*nl_filter)(struct nlmsghdr *nlh, void *arg);
typedef int	(*nl_print)(unsigned long *parg);

/*********************************************************
 *	Neight(ARP) functions.
 ********************************************************/
extern int
nl_neigh_add();

extern int 
nl_neigh_del();

extern int 
nl_neight_flush();

extern int 
nl_neight_find();

extern int 
nl_neigh_list();

/*********************************************************
 *	IP address functions.
 ********************************************************/
extern int 
nl_addr_add(int index, int family, void *addr, int cidr);

extern int 
nl_addr_replace(int index, int family, void *addr, int cidr);

extern int 
nl_addr_modify(int index, int family, void *addr, int cidr, void *old, int ocidr);

extern int 
nl_addr_delete(int index, int family, void *addr, int cidr);

extern int 
nl_addr_flush(int index, int family);

extern int 
nl_addr_find(int index, int family, void *addr);

extern int 
nl_addr_list(int index, int family, nl_print print, void *arg);

/*********************************************************
 *	Route address functions.
 ********************************************************/
extern int 
nl_route_add();

extern int 
nl_route_del();

extern int 
nl_route_flush();

extern int 
nl_route_find();

extern int 
nl_route_list();

#endif /* end of FZ_NETLINK_H  */


