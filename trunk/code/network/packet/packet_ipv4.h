/**
 *	@file	packet_ipv4.h
 *
 *	@brief	IPv4 packet decode function.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PACKET_IPV4_H
#define FZ_PACKET_IPV4_H

#include "netpkt.h"

/**
 *	Decode IPv4 header in @pkt.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
ipv4_decode(netpkt_t *pkt);

/**
 *	Get IPv4 checksum of header buffer @w,
 *	@w length is @len.
 *
 *	@len must >= 10 and <= 30 and must be even number.
 *
 * 	Return >= 0 if success, -1 on error.
 */
extern int 
ipv4_cksum(const u_int16_t *w, size_t len);

/**
 *	Print IPv4 header in @pkt.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
ipv4_print(const netpkt_t *pkt, const char *prefix);


#endif /* end of FZ_PACKET_IPV4_H */


