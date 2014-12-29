/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <arpa/inet.h>

#include "gcc_common.h"
#include "dbg_common.h"

#include "packet_icmpv6.h"


int 
icmpv6_decode(netpkt_t *pkt)
{
	int n;
	int m;

	if (unlikely(!pkt))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(pkt->hdr4_type != IPPROTO_ICMPV6))
		ERR_RET(-1, "layer 4 type is error");

	if (unlikely(!pkt->head))
		ERR_RET(-1, "no data in packet\n");

	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len + pkt->hdr3_len;
	if ((n -m) < sizeof(struct icmp6_hdr))
		ERR_RET(-1, "invalid ICMPv6 header\n");

	pkt->hdr4_icmpv6 = (struct icmp6_hdr *)(pkt->head + m);
	pkt->hdr4_len = sizeof(struct icmp6_hdr);

	return 0;
}


int  
icmpv6_print(const netpkt_t *pkt, const char *prefix)
{
	int n;
	int m;
	struct icmp6_hdr *h;
	char s[INET6_ADDRSTRLEN];
	struct nd_neighbor_solicit *nd_ns;
	struct nd_neighbor_advert *nd_na;

	if (unlikely(!pkt || !prefix))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(pkt->hdr4_type != IPPROTO_ICMPV6))
		ERR_RET(-1, "layer 4 type is error\n");

	if (unlikely(!pkt->hdr4_icmpv6))
		ERR_RET(-1, "not decode ICMPv6 header\n");

	h = pkt->hdr4_icmpv6;
	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len + pkt->hdr3_len + pkt->hdr4_len;
	printf("%slen:            %d\n", prefix, pkt->hdr4_len);
	printf("%sdata:           %d\n", prefix, (n - m));
	printf("%stype:           %u\n", prefix, h->icmp6_type);
	printf("%scode:           %u\n", prefix, h->icmp6_code);
	printf("%schsum:          %u\n", prefix, ntohs(h->icmp6_cksum));

	switch (h->icmp6_type) {
	
	case ICMP6_ECHO_REQUEST:
		printf("%sid:             %u\n", prefix, ntohs(h->icmp6_id));
		printf("%sseq:            %u\n", prefix, ntohs(h->icmp6_seq));
		break;
	case ICMP6_ECHO_REPLY:
		printf("%sid:             %u\n", prefix, ntohs(h->icmp6_id));
		printf("%sseq:            %u\n", prefix, ntohs(h->icmp6_seq));
		break;
	case ND_NEIGHBOR_SOLICIT:
		nd_ns = (struct nd_neighbor_solicit *)h;
		printf("%sreserved:       %u\n", prefix, nd_ns->nd_ns_reserved);
		printf("%starget:         %s\n", prefix, 
			inet_ntop(AF_INET6, &nd_ns->nd_ns_target, s, sizeof(s)));
		break;
	case ND_NEIGHBOR_ADVERT:
		nd_na = (struct nd_neighbor_advert *)h;
		printf("%sreserved:%u\n", prefix, 
			nd_na->nd_na_flags_reserved);
		printf("%starget:\t%s\n", prefix, 
			inet_ntop(AF_INET6, &nd_na->nd_na_target, s, sizeof(s)));
	default:
		break;
	}

	return 0;
}

