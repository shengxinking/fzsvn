/**
 *	@file	packet_arp.h
 *
 *	@brief	ARP decode and encode function.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2014-12-14
 */

#include "gcc_common.h"
#include "dbg_common.h"

#include "packet_arp.h"

#ifndef	NIPQUAD
#define NIPQUAD(ip)    \
	((unsigned char *)&ip)[0], \
	((unsigned char *)&ip)[1], \
	((unsigned char *)&ip)[2], \
	((unsigned char *)&ip)[3]
#endif

int 
arp_decode(netpkt_t *pkt)
{
	int n;
	int m;
	int arplen;

	if (unlikely(!pkt))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(!pkt->head))
		ERR_RET(-1, "no data in packet\n");

	if (unlikely(pkt->hdr3_type != ETHERTYPE_ARP))
		ERR_RET(-1, "invalid layer 3 type\n");

	/* check data length */
	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len;
	arplen = sizeof(struct arphdr) + sizeof(arp_payload_t);
	if (unlikely((n - m) < arplen))
		ERR_RET(-1, "invalid arp header\n");

	/* get arp header */
	pkt->hdr3_arp = (struct arphdr*)(pkt->head + pkt->hdr2_len);
	pkt->hdr3_len = sizeof(struct arphdr);

	return 0;
}


int 
arp_print(const netpkt_t *pkt, const char *prefix)
{
	int n;
	int m;
	struct arphdr *h;
	arp_payload_t *payload;

	if (unlikely(!pkt || !prefix))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(pkt->hdr3_type != ETHERTYPE_ARP))
		ERR_RET(-1, "invalid layer 3 type\n");

	if (unlikely(!pkt->hdr3_arp))
		ERR_RET(-1, "not decode layer 3 header\n");

	h = pkt->hdr3_arp;
	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len + pkt->hdr3_len + sizeof(arp_payload_t);
	printf("%slen:            %d\n", prefix, pkt->hdr3_len);
	printf("%sdata:           %d\n", prefix, (n - m));
	printf("%sar_hdr:         %u\n", prefix, h->ar_hrd);
	printf("%sar_pro:         %u\n", prefix, h->ar_pro);
	printf("%sar_hln:         %u\n", prefix, h->ar_hln);
	printf("%sar_pln:         %u\n", prefix, h->ar_pln);
	printf("%sar_op:          %u\n", prefix, h->ar_op);
	
	/* print arp payload */
	payload = (arp_payload_t *)(pkt->head + pkt->hdr2_len + pkt->hdr3_len);
	printf("%ssha:            %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", 
	       prefix, 
	       payload->sha[0],
	       payload->sha[1],
	       payload->sha[2],
	       payload->sha[3],
	       payload->sha[4],
	       payload->sha[5]);

	printf("%ssip:            %d.%d.%d.%d\n", prefix, NIPQUAD(payload->sip));
	printf("%sdha:            %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", 
	       prefix, 
	       payload->dha[0],
	       payload->dha[1],
	       payload->dha[2],
	       payload->dha[3],
	       payload->dha[4],
	       payload->dha[5]);
	printf("%sdip:            %d.%d.%d.%d\n", prefix, NIPQUAD(payload->dip));

	return 0;
}




