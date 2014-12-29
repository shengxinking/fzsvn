/**
 *	@file	packet_util.h
 *
 *	@brief	APIs to create Ethernet packet, IP packet, 
 *		ICPM packet, UDP packet, TCP packet.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2013-09-26
 */

#ifndef FZ_PACKET_UTIL_H
#define FZ_PACKET_UTIL_H

extern int 
packet_ether();

extern int 
packet_ip();

extern int 
packet_icmp();

extern int 
packet_udp();

extern int 
packet_tcp();

extern int 
packet_tcp_checksum();

extern int 
packet_udp_checksum();

extern int 
packet_icmp_checksum();


#endif /* end of FZ_  */


