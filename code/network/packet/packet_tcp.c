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

#include "packet_tcp.h"

int 
tcp_decode(netpkt_t *pkt)
{
	int n;

	if (unlikely(!pkt))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(pkt->hdr4_type != IPPROTO_TCP))
		ERR_RET(-1, "not TCP packet\n");

	if (unlikely(!pkt->head))
		ERR_RET(-1, "no data in packet\n");

	n = pkt->hdr2_len + pkt->hdr3_len;
	pkt->hdr4_tcp = (struct tcphdr *)(pkt->head + n);
	
	pkt->hdr4_len = (size_t)pkt->hdr4_tcp->doff * 4;

	return 0;
}

int 
tcp_print(const netpkt_t *pkt, const char *prefix)
{
	int n;
	int m;
	struct tcphdr *h;

	if (unlikely(!pkt || !prefix))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(pkt->hdr4_type != IPPROTO_TCP))
		ERR_RET(-1, "invalid layer 4 type\n");

	if (unlikely(!pkt->hdr4_tcp))
		ERR_RET(-1, "not decode tcp4 header\n");

	h = pkt->hdr4_tcp;
	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len + pkt->hdr3_len + pkt->hdr4_len;
	printf("%slen:            %d\n", prefix, pkt->hdr4_len);
	printf("%sdata:           %d\n", prefix, (n - m));
	printf("%ssource:         %u\n", prefix, ntohs(h->source));
	printf("%sdest:           %u\n", prefix, ntohs(h->dest));
	printf("%sseq:            %u\n", prefix, ntohl(h->seq));
	printf("%sack:            %u\n", prefix, ntohl(h->ack_seq));
	printf("%sres1:           %u\n", prefix, h->res1);
	printf("%sdoff:           %u\n", prefix, h->doff);
	printf("%sfin:            %u\n", prefix, h->fin);
	printf("%ssyn:            %u\n", prefix, h->syn);
	printf("%srst:            %u\n", prefix, h->rst);
	printf("%spsh:            %u\n", prefix, h->psh);
	printf("%sack:            %u\n", prefix, h->ack);
	printf("%surg:            %u\n", prefix, h->urg);
	printf("%sres2:           %u\n", prefix, h->res2);
	printf("%swin:            %u\n", prefix, ntohs(h->window));
	printf("%scheck:          %u\n", prefix, ntohs(h->check));
	printf("%surg_ptr:        %u\n", prefix, ntohs(h->urg_ptr));
	
	return 0;
}

