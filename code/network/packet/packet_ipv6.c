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

#include "packet_ipv6.h"


int 
ipv6_decode(netpkt_t *pkt)
{
	int n;
	int m;
	u_int8_t nexthdr;
	struct ip6_ext *ext;

	if (unlikely(!pkt))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(!pkt->head))
		ERR_RET(-1, "not data in packet\n");

	if (unlikely(pkt->hdr3_type != ETHERTYPE_IPV6))
		ERR_RET(-1, "hdr3 type is not IPv6\n");

	n = pkt->hdr2_len;
	pkt->hdr3_ipv6 = (struct ip6_hdr *)(pkt->head + n);
	pkt->hdr3_len = sizeof(struct ip6_hdr);

	/* check version */
	if (((pkt->hdr3_ipv6->ip6_flow & 0xff) >> 4) != 6)
		ERR_RET(-1, "invalid IPv6 header version\n");

	/* parse IPv6 extension header */
	m = 0;
	n = pkt->hdr2_len + pkt->hdr3_len;
	nexthdr = pkt->hdr3_ipv6->ip6_nxt;
	while (IP6HDR_IS_EXT(nexthdr)) {
		ext = (struct ip6_ext *)(pkt->head + n + m);
		nexthdr = ext->ip6e_nxt;

		/* fragment */
		if (nexthdr == IPPROTO_FRAGMENT) {
			pkt->frag_id  = ntohl(*(u_int32_t *)(pkt->head + n + 4));
			pkt->frag_off = ntohs(*(u_int16_t *)(pkt->head + n + 2));
			m += 8;
		}
		else {
			m += (8 + (ext->ip6e_len << 3));
		}
	}

	pkt->hdr3_len += m;
	pkt->hdr4_type = nexthdr;

	return 0;
}

int 
ipv6_print(const netpkt_t *pkt, const char *prefix)
{
	struct ip6_hdr *h;
	char ipstr[INET6_ADDRSTRLEN];

	if (unlikely(!pkt || !prefix))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(!pkt->head))
		ERR_RET(-1, "no data in packet\n");

	if (unlikely(pkt->hdr3_type != ETHERTYPE_IPV6))
		ERR_RET(-1, "not IPv6 packet\n");

	if (unlikely(!pkt->hdr3_ipv6))
		ERR_RET(-1, "not decode IPv6 header\n");

	h = pkt->hdr3_ipv6;

	printf("%slen:            %d\n", prefix, pkt->hdr3_len);
	printf("%sver:            %u\n", prefix, IP6_VER(h->ip6_flow));
	printf("%sclass:          %u\n", prefix, IP6_CLASS(h->ip6_flow));
	printf("%sflow:           %u\n", prefix, IP6_FLABLE(h->ip6_flow));
	printf("%splen:           %u\n", prefix, ntohs(h->ip6_plen));
	printf("%snxt:            %u\n", prefix, h->ip6_nxt);
	printf("%shlim:           %u\n", prefix, h->ip6_hlim);
	printf("%ssrc:            %s\n", prefix, 
		inet_ntop(AF_INET6, &h->ip6_src, ipstr, sizeof(ipstr)));
	printf("%sdst:            %s\n", prefix, 
		inet_ntop(AF_INET6, &h->ip6_dst, ipstr, sizeof(ipstr)));

	return 0;
}


