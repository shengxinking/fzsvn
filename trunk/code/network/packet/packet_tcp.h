/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PACKET_TCP_H
#define FZ_PACKET_TCP_H

#include "netpkt.h"

/**
 *	Decode tcp header in @pkt.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
tcp_decode(netpkt_t *pkt);

/**
 *	Print tcp header in @pkt.
 *
 * 	No return.
 */
extern int 
tcp_print(const netpkt_t *pkt, const char *prefix);



#endif /* end of FZ_PACKET_TCP_H */


