/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PACKET_ICMPV6_H
#define FZ_PACKET_ICMPV6_H

#include "netpkt.h"


/**
 *	Decode ICMPv6 header in @pkt.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
icmpv6_decode(netpkt_t *pkt);

/**
 *	Print ICMPv6 header in @pkt.
 *
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
icmpv6_print(const netpkt_t *pkt, const char *prefix);

#endif /* end of FZ_ICMPV6_H */


