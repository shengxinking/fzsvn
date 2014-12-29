/**
 *	@file	netssn.h
 *
 *	@brief	Network packet session for layer4 protocol, like socket.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_NETSSN_H
#define FZ_NETSSN_H

typedef struct netssn {
	cblist_t	list;	/* in list */
	ip_addr_t	src;	/* src address */
	ip_addr_t	dst;	/* dst address */
	u_int16_t	sport;	/* source port */
	u_int16_t	dport;	/* dest port */
	cblist_t	inlist;	/* input netpkt list */
	cblist_t	outlist;/* output netpkt list */

	/* tcp options */
	u_int32_t	seq;	/* TCP sequence */
	u_int32_t	ack;	/* TCP ack */
	u_int32_t	win;	/* TCP window size */
} netssn_t;


#endif /* end of FZ_NETSSN_H */


