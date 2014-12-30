/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_NETPKT_H
#define FZ_NETPKT_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

#include "cblist.h"

#define	IPV4_HLEN		20

/* IPv4 fragment macro */
#define	IPV4_FRAG_DF(off)	(((off) & IP_DF) >> 14)
#define	IPV4_FRAG_MF(off)	(((off) & IP_MF) >> 13)
#define	IPV4_FRAG_OFF(off)	((off) & IP_OFFMASK)

/* IPv6 fragment macro */
#define	IPV6_FRAG_MF(off)	((off) & IP6F_MORE_FRAG >> 15)
#define	IPV6_FRAG_OFF(off)	((off) & IP6F_OFF_MASK)

/**
 *	The network packet common structure.
 *
 */
typedef struct netpkt {
	cblist_t		list;	/* in list */

	struct ether_header	*eth;	/* ether header */
	u_int32_t		hdr2_len;/* layer 2 length */

	/* vlan flags */
	u_int16_t		vlan_pcp:3;
	u_int16_t		vlan_cfi:1;
	u_int16_t		vlan_vid:12;
	
	/* layer 3 */
	u_int32_t		hdr3_type;/* layer 3 family */
	union {
		struct arphdr	*arp;	/* arp header */
		struct iphdr	*ipv4;	/* IPv4 header */
		struct ip6_hdr	*ipv6;	/* IPv6 header */
	} hdr3;				/* layer 3 header */

	u_int32_t		frag_id;/* fragment identification for IPv6 */
	u_int16_t		frag_off;/* fragment offset for IPv6 */

	u_int16_t		hdr3_len;/* layer 3 length */
	
	/* layer 4 */
	u_int32_t		hdr4_type;/* layer 4 protocol */
	union {
		struct icmphdr	*icmpv4;/* icmpv4 header */
		struct icmp6_hdr *icmpv6;/* icmpv6 header */
		struct udphdr	*udp;	/* udp header */
		struct tcphdr	*tcp;	/* tcp header */
	} hdr4;				/* layer 4 header */
	u_int32_t		hdr4_len;/* layer 4 length */

	/* raw data */
	u_int8_t		*start;	/* start of memory */
	u_int8_t		*end;	/* end of memory */
	u_int8_t		*head;	/* head of packet data */
	u_int8_t		*tail;	/* tail of packet data */
} netpkt_t;

#define	hdr3_arp	hdr3.arp
#define	hdr3_ipv4	hdr3.ipv4
#define	hdr3_ipv6	hdr3.ipv6

#define	hdr4_icmpv4	hdr4.icmpv4
#define	hdr4_icmpv6	hdr4.icmpv6
#define	hdr4_udp	hdr4.udp
#define	hdr4_tcp	hdr4.tcp

/**
 *	Init @pkt, the data is @data, length is @len.
 *
 * 	Return 0 if success, -1 on error.
 */
static inline int 
netpkt_init(netpkt_t *pkt, const void *data, size_t size)
{
	memset(pkt, 0, sizeof(*pkt));
	CBLIST_INIT(&pkt->list);
	pkt->start = (u_int8_t *)data;
	pkt->end = (u_int8_t *)data + size;
	pkt->head = (u_int8_t *)data;
	pkt->tail = (u_int8_t *)data;

	return 0;
}

/**
 *	Alloc a netpkt_t object and return it.
 *
 *	Return pointer if success, NULL on error.
 */
static inline netpkt_t * 
netpkt_alloc(size_t size)
{
	size_t n;
	u_int8_t *data;
	netpkt_t *pkt;
	
	n = sizeof(*pkt) + size;
	pkt = malloc(n);
	if (!pkt)
		return NULL;

	if (size)
		data = (u_int8_t *)pkt + sizeof(*pkt);
	else
		data = NULL;

	netpkt_init(pkt, data, size);

	return pkt;
}

/**
 *	Free @pkt which alloced by @netpkt_alloc().
 *
 *	No return.
 */
static inline void 
netpkt_free(netpkt_t *pkt)
{
	/* delete from list */
	CBLIST_DEL(&pkt->list);

	free(pkt);
}

/**
 *	Get the head room length of @pkt.
 *
 * 	Return >= 0 if success, < 0 on error.
 */
static inline int 
netpkt_headroom(const netpkt_t *pkt)
{
	return (pkt->head - pkt->start);
}

/**
 *	Get the tail room length of @pkt.
 *
 * 	Return >= 0 if success, < 0 on error.
 */
static inline int 
netpkt_tailroom(const netpkt_t *pkt)
{
	return (pkt->end - pkt->tail);
}

/**
 *	Reserve @len bytes in @pkt headroom. It'll move data
 *	if needed.
 *
 *	Return 0 if success, -1 on error.
 */
static inline int 
netpkt_reserve(netpkt_t *pkt, int len)
{
	int n;
	int m;
	int headroom;
	int tailroom;

	if (len < 1)
		return -1;

	headroom = netpkt_headroom(pkt);
	if (headroom < 0)
		return -1;

	/* head room enough */
	if (headroom >= len)
		return 0;

	tailroom = netpkt_tailroom(pkt);
	if (tailroom < 0)
		return -1;

	/* no room */
	if (len > (headroom + tailroom))
		return -1;

	/* no data move */
	if (pkt->tail == pkt->head) {
		pkt->head = pkt->start + len;
	}
	/* need move data, expensive operator */
	else {
		m = pkt->tail - pkt->head;
		if (m < 1)
			return -1;

		n = len - headroom;
		memmove(pkt->head, pkt->head + n, m);
		pkt->head += n;
		pkt->tail += n;
	}

	return 0;
}

/**
 *	Push data into @pkt headroom.
 *
 *	Return 0 if success, -1 on error.
 */
static inline int 
netpkt_push_data(netpkt_t *pkt, const u_int8_t *buf, size_t len)
{
	if (!buf || len < 1)
		return -1;

	if (len >= netpkt_headroom(pkt))
		return -1;

	pkt->head -= len;
	memcpy(pkt->head, buf, len);
	return 0;
}

/**
 *	Add data into @pkt tailroom.
 *
 *	Return 0 if success, -1 on error.
 */
static inline int 
netpkt_add_data(netpkt_t *pkt, const u_int8_t *buf, size_t len)
{
	if (!buf || len < 1)
		return -1;

	/* check tail room */
	if (len > netpkt_tailroom(pkt))
		return -1;

	memcpy(pkt->tail, buf, len);
	pkt->tail += len;
	return 0;
}

static inline void 
netpkt_print(const netpkt_t *pkt, const char *prefix)
{
	printf("%sstart:\t%p\n",  prefix, pkt->start);
	printf("%send:\t%p\n",    prefix, pkt->end);
	printf("%shead:\t%p\n",   prefix, pkt->head);
	printf("%stail:\t%p\n",   prefix, pkt->tail);
	printf("%sheadroom:%d\n", prefix, netpkt_headroom(pkt));
	printf("%stailroom:%d\n", prefix, netpkt_tailroom(pkt));
	printf("%smemsize:%lu\n",  prefix, (pkt->end - pkt->start));
	printf("%sdatasize:%lu\n", prefix, (pkt->tail - pkt->head));
}

#endif /* end of FZ_NETPKT_H */


