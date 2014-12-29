/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include "gcc_common.h"
#include "dbg_common.h"

#include "packet_udp.h"

int
udp_decode(netpkt_t *pkt)
{
	int n;
	int m;

	/* check argument */
	if (unlikely(!pkt))
		ERR_RET(-1, "invalid argument\n");

	/* check type */
	if (unlikely(pkt->hdr4_type != IPPROTO_UDP))
		ERR_RET(-1, "invalid layer 4 type\n");

	/* check data */
	if (unlikely(!pkt->head))
		ERR_RET(-1, "no data in packet\n");

	/* check length */
	n = pkt->hdr2_len + pkt->hdr3_len;
	m = pkt->tail - pkt->head;
	if (unlikely((m - n) < sizeof(struct udphdr)))
		ERR_RET(-1, "invalid UDP header\n");

	pkt->hdr4_udp = (struct udphdr *)(pkt->head + n);

	return 0;
}

int 
udp_print(const netpkt_t *pkt, const char *prefix)
{
	int n;
	int m;
	struct udphdr *h;

	if (unlikely(!pkt || !prefix))
		ERR_RET(-1, "invalid argument\n");

	/* check type */
	if (unlikely(pkt->hdr4_type != IPPROTO_UDP))
		ERR_RET(-1, "invalid layer 4 type\n");

	if (unlikely(!pkt->hdr4_udp))
		ERR_RET(-1, "not decode UDP header\n");

	h = pkt->hdr4_udp;
	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len + pkt->hdr3_len + pkt->hdr4_len;
	printf("%slen:            %d\n", prefix, pkt->hdr4_len);
	printf("%sdata:           %d\n", prefix, (n - m));
	printf("%ssport:          %u\n", prefix, ntohs(h->source));
	printf("%sdport:          %u\n", prefix, ntohs(h->dest));
	printf("%slen:            %u\n", prefix, ntohs(h->len));
	printf("%scksum:          %u\n", prefix, ntohs(h->check));

	return 0;
}


