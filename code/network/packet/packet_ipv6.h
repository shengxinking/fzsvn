/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PACKET_IPV6_H
#define FZ_PACKET_IPV6_H

#include "netpkt.h"

#define	IP6HDR_IS_EXT(type)	\
	((type) == IPPROTO_HOPOPTS || \
	 (type) == IPPROTO_ROUTING || \
	 (type) == IPPROTO_FRAGMENT || \
	 (type) == IPPROTO_DSTOPTS || \
	 (type) == IPPROTO_MH)

#define	IP6_VER(flow)			\
	((ntohl(flow)) >> 28)
#define	IP6_CLASS(flow)			\
	((ntohl(flow) >> 20) & 0xff)
#define	IP6_FLABLE(flow)		\
	(ntohl(flow) & 0xfffff)
	

/**
 *	Decode IPv6 header in @pkt.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ipv6_decode(netpkt_t *pkt);

/**
 *	Print IPv6 header in @pkt.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
ipv6_print(const netpkt_t *pkt, const char *prefix);


#endif /* end of FZ_PACKET_IPV6_H */


