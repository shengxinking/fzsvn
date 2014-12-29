/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PACKET_ICMPV4_H
#define FZ_PACKET_ICMPV4_H

#include "netpkt.h"


/**
 *	Decode ICMPv4 header.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
icmpv4_decode(netpkt_t *pkt);

/**
 *	Printer ICMPv4 header.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
icmpv4_print(const netpkt_t *pkt, const char *prefix);


#endif /* end of FZ_ICMPV4_H */


