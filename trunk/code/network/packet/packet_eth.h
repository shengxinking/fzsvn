/**
 *	@file	packet_eth.h
 *
 *	@brief	Ethernet header decode/encode function.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2014-12-12
 */

#ifndef FZ_PACKET_ETH_H
#define FZ_PACKET_ETH_H

#include "netpkt.h"

/**
 *	Decode ethernet header in @pkt
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
eth_decode(netpkt_t *pkt);

/**
 *	Print the ethernet header in @pkt
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
eth_print(const netpkt_t *pkt, const char *prefix);

#endif /* end of FZ_PACKET_ETH_H */


