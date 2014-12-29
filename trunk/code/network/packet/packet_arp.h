/**
 *	@file	packet_arp.h
 *
 *	@brief	ARP packet decode/encode function
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PACKET_ARP_H
#define FZ_PACKET_ARP_H

#include "netpkt.h"

typedef struct arp_payload {
	u_int8_t	sha[ETH_ALEN];
	u_int32_t	sip;
	u_int8_t	dha[ETH_ALEN];
	u_int32_t	dip;
} __attribute__ ((__packed__)) arp_payload_t;

/**
 *	Decode ARP header in packet @pkt.
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
arp_decode(netpkt_t *pkt);

/**
 *	Print ARP header and payload
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
arp_print(const netpkt_t *pkt, const char *prefix);



#endif /* end of FZ_PACKET_ARP_H */


