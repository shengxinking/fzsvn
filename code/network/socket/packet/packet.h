/**
 *
 *	@file	packet.h
 *	@brief	A packet socket API, it using AF_PACKET 
 *		socket to send/recv packet.
 *
 *	@author	write by Forrest.zhang
 */

#ifndef FZ_PACKET_H
#define FZ_PACKET_H

#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/types.h>
#include <sys/socket.h>

#define IP_ALEN              4          /* INET address size */

extern const char * 
packet_error(void);

extern u_int16_t 
packet_checksum(const u_int16_t *ptr, int size);

extern u_int32_t 
packet_aton(const char *address);

/**
 *	Create a AF_PACKET socket, and the protocol is SOCK_RAW
 *
 */
extern int 
packet_socket(void);

/**  
 *	Send a packet from packet socket @fd, the packet is send
 *	through interface @ifindex, the destination MAC address
 *	is @dmac.
 *
 *	Return 0 if send packet success, -1 on error.
 */
extern int 
packet_send(int	fd, u_int8_t *pkt, size_t plen, int ifindex, u_int8_t              *dmac);

/**
 *	Recv packet from packet socket, the recved packet is stored in
 *	@pkt, the recved packet length is @plen
 *
 *	Return 0 if recv success, -1 on error.
 */
extern int 
packet_recv(int fd; 	u_int8_t *buf, size_t *plen);

/**
 *	proto is ETH_P_ARP or ETH_P_IP etc. see <linux/if_ether.h>
 */
extern int packet_ether(
	const u_int8_t           *buf, 
	int                      buf_len, 
	u_int8_t                 *pkt,
	int                      pkt_len,
	const char               *smac,
	const char               *dmac,
	int                      proto);

/**
 *	Create a IP packet.
 */
extern int packet_ip(
	const u_int8_t           *buf, 
	int                      buf_len, 
	u_int8_t                 *pkt,
	int                      pkt_len,
	u_int32_t                sip,
	u_int32_t                dip,
	u_int8_t                 protocol);
    
/**
 *	Create a ICMP echo packet using @id, @sequence, @type, the build packet
 *	is stored in pkt, packet length is @pkt_len, the ICMP echo data is stored
 *	in @buf, length is @blen
 */
extern int packet_icmp_echo(
	const u_int8_t           *buf,
	int                      blen,
	u_int8_t                 type,
	u_int16_t                id,
	u_int16_t                sequence,
	u_int8_t                 *pkt,
	int                      *plen);

/**
 *	Create a tcp packet.
 */
extern int packet_tcp(
	const u_int8_t           *buf, 
	int                      buf_len,
	u_int8_t                 *pkt,
	int                      pkt_len,
	u_int16_t                sport,
	u_int16_t                dport,
	int                      type);

/**
 * 	Create a udp packet.
 */
extern int packet_udp(
	const u_int8_t           *buf, 
	int                      buf_len,
	u_int16_t                sport,
	u_int16_t                dport,
	u_int32_t                sip,
	u_int32_t                dip,
	u_int8_t                 protocol
	u_int8_t                 *pkt,
	int                      pkt_len,
);
    
/**
 *	Create a ARP packet, and stored packet in @pkt, packet length is @plen.
 *	if @req is not zero, it'll create ARP request packet, else build ARP
 *	response packet.
 */
extern int packet_arp(
	u_int8_t	*pkt,
	int		*len,
	const char	*smac,
	u_int32_t	sip,
	const char	*dmac,
	u_int32_t	dip,
	int		req);
	


#endif /* end of __PACKET_H__ */
