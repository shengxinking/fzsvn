/**
 *	@file	netlink.h
 *
 *	@brief	Netlink APIs.
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
nl_add_neigh();

extern int 
nl_del_neigh();

extern int 
nl_flush_neight();

extern int 
nl_find_neight();

extern int 
nl_list_neigh();

/*********************************************************
 *	IP address functions.
 ********************************************************/
extern int 
nl_add_addr(int index, int family, void *addr, int cidr);

extern int 
nl_replace_addr(int index, int family, void *addr, int cidr);

extern int 
nl_modify_addr(int index, int family, void *addr, int cidr, void *old, int ocidr);

extern int 
nl_delete_addr(int index, int family, void *addr, int cidr);

extern int 
nl_flush_addr(int index, int family);

extern int 
nl_find_addr(int index, int family, void *addr);

extern int 
nl_list_addr(int index, int family, nl_print print, void *arg);

/*********************************************************
 *	Route address functions.
 ********************************************************/
extern int 
nl_add_route();

extern int 
nl_del_route();

extern int 
nl_flush_route();

extern int 
nl_find_route();

extern int 
nl_list_route();

/********************************************************
 * 	Rules function 
 *******************************************************/
extern int 
nl_add_rule();

extern int 
nl_del_rule();

extern int 
nl_find_rule();

extern int 
nl_list_rule();

#endif /* end of FZ_NETLINK_H  */


