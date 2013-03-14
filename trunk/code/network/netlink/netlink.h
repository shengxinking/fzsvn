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

/*********************************************************
 *	Neight(ARP) functions.
 ********************************************************/
extern int
nl4_add_neight();

extern int 
nl6_add_neight();

extern int 
nl4_del_neight();

extern int 
nl6_del_neight();

extern int 
nl4_flush_neight();

extern int 
nl6_flush_neight();

extern int 
nl4_find_neight();

extern int 
nl6_find_neight();

/*********************************************************
 *	IP address functions.
 ********************************************************/
extern int 
nl4_add_ip();

extern int 
nl6_add_ip();

extern int 
nl4_del_ip();

extern int 
nl6_del_ip();

extern int 
nl4_flush_ip();

extern int 
nl6_flush_ip();

extern int 
nl4_find_ip();

extern int 
nl6_find_ip();

/*********************************************************
 *	Route address functions.
 ********************************************************/
extern int 
nl4_add_route();

extern int 
nl6_add_route();

extern int 
nl4_del_route();

extern int 
nl6_del_route();

extern int 
nl4_flush_route();

extern int 
nl6_flush_route();

extern int 
nl4_find_route();

extern int 
nl6_find_route();


#endif /* end of FZ_NETLINK_H  */


