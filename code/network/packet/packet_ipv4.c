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

#include "packet_ipv4.h"

#ifndef	NIPQUAD
#define NIPQUAD(ip)    \
	((unsigned char *)&ip)[0], \
	((unsigned char *)&ip)[1], \
	((unsigned char *)&ip)[2], \
	((unsigned char *)&ip)[3]
#endif

int 
ipv4_decode(netpkt_t *pkt)
{
	int n;
	int m;

	if (unlikely(!pkt))
		ERR_RET(-1, "invalid argument\n");

	/* check layer 3 type */
	if (unlikely(pkt->hdr3_type != ETHERTYPE_IP))
		ERR_RET(-1, "not IPv4 header\n");

	if (unlikely(!pkt->head))
		ERR_RET(-1, "no data in packet\n");

	/* calc hdr3 data */
	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len;
	if ((n - m) < sizeof(struct iphdr))
		ERR_RET(-1, "invalid IPv4 header\n");

	/* get layer 3 header and length */
	pkt->hdr3_ipv4 = (struct iphdr *)(pkt->head + pkt->hdr2_len);
	pkt->hdr3_len = pkt->hdr3_ipv4->ihl * 4;

	if (unlikely(pkt->hdr3_len < IPV4_HLEN))
		ERR_RET(-1, "ip header length is too small\n");

	/* get layer 4 type */
	pkt->hdr4_type = pkt->hdr3_ipv4->protocol;

	return 0;
}

int 
ipv4_cksum(const u_int16_t *w, size_t len)
{
	int i;
	u_int32_t cksum = 0;

	if (unlikely(len < 10 || len > 30 || len % 2))
		ERR_RET(-1, "invalid header length\n");

	/* get check sum */
	for (i = 0; i < len; i++)
		cksum += w[i];

	cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
	cksum += (cksum >> 16);

	return (u_int16_t)(~cksum);
}

int 
ipv4_print(const netpkt_t *pkt, const char *prefix)
{
	int n;
	int m;
	struct iphdr *h;

	if (unlikely(!pkt || !prefix))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(pkt->hdr3_type != ETHERTYPE_IP))
		ERR_RET(-1, "layer 3 type is error\n");

	if (unlikely(!pkt->hdr3_ipv4))
		ERR_RET(-1, "not decode layer 3 header\n");
	
	h = pkt->hdr3_ipv4;
	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len + pkt->hdr3_len;

	printf("%slen:            %d\n", prefix, pkt->hdr3_len);
	printf("%sdata:           %d\n", prefix, (n - m));
	printf("%sver:            %d\n", prefix, h->version);
	printf("%sihl:            %d\n", prefix, h->ihl);
	printf("%stos:            %x\n", prefix, h->tos);
	printf("%slen:            %d\n", prefix, ntohs(h->tot_len));
	printf("%sid:             %x\n", prefix, ntohs(h->id));
	printf("%sfrag_df:        %x\n", prefix, IPV4_FRAG_DF(ntohs(h->frag_off)));
	printf("%sfrag_mf:        %x\n", prefix, IPV4_FRAG_MF(ntohs(h->frag_off)));
	printf("%sfrag_off:       %x\n", prefix, IPV4_FRAG_OFF(ntohs(h->frag_off)));
	printf("%sttl:            %x\n", prefix, h->ttl);
	printf("%sproto:          %x\n", prefix, h->protocol);
	printf("%scksum:          %x\n", prefix, ntohs(h->check));
	printf("%ssrc:            %d.%d.%d.%d\n", prefix, NIPQUAD(h->saddr));
	printf("%sdst:            %d.%d.%d.%d\n", prefix, NIPQUAD(h->daddr));

	return 0;
}

