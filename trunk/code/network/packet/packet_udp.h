/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PACKET_UDP_H
#define FZ_PACKET_UDP_H

#include "netpkt.h"

/**
 *	Decode UDP header in @pkt
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
udp_decode(netpkt_t *pkt);

/**
 *	Get the UDP checksum.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
udp_cksum(netpkt_t *pkt);

/**
 *	Print UDP header and payload.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
udp_print(const netpkt_t *pkt, const char *prefix);


#endif /* end of FZ_PACKET_UDP_H */


