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

#include "packet_icmpv4.h"


int 
icmpv4_decode(netpkt_t *pkt)
{
	int n;

	if (unlikely(!pkt))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(pkt->hdr4_type != IPPROTO_ICMP))
		ERR_RET(-1, "layer 4 type is error\n");

	if (unlikely(!pkt->head))
		ERR_RET(-1, "no data in packet\n");

	/* check length */
	n = (pkt->tail - pkt->head) - (pkt->hdr2_len + pkt->hdr3_len);
	if (n < sizeof(struct icmphdr))
		ERR_RET(-1, "not enough data for ICMP4 header\n");

	n = pkt->hdr2_len + pkt->hdr3_len;
	pkt->hdr4_icmpv4 = (struct icmphdr *)(pkt->head + n);
	pkt->hdr4_len = sizeof(struct icmphdr);

	return 0;
}


int 
icmpv4_print(const netpkt_t *pkt, const char *prefix)
{
	int n;
	int m;
	struct icmphdr *h;

	if (unlikely(!pkt || !prefix))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(pkt->hdr4_type != IPPROTO_ICMP))
		ERR_RET(-1, "layer 4 type is error\n");

	if (unlikely(!pkt->hdr4_icmpv4))
		ERR_RET(-1, "not decode ICMPv4 header\n");
	
	h = pkt->hdr4_icmpv4;
	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len + pkt->hdr3_len + pkt->hdr4_len;
	printf("%slen:            %d\n", prefix, pkt->hdr4_len);
	printf("%sdata:           %d\n", prefix, (n - m));
	printf("%stype:           %u\n", prefix, h->type);
	printf("%scode:           %u\n", prefix, h->code);
	printf("%schecksum:       %u\n", prefix, h->checksum);
	
	switch (pkt->hdr4_icmpv4->type) {
		case ICMP_ECHOREPLY:
			printf("%sreply id:       %u\n", 
				prefix, ntohs(h->un.echo.id));
			printf("%sreply seq:      %u\n", 
				prefix, ntohs(h->un.echo.sequence));
			break;
		case ICMP_ECHO:
			printf("%secho id:        %u\n", 
				prefix, ntohs(h->un.echo.id));
			printf("%secho seq:       %u\n", 
				prefix, ntohs(h->un.echo.sequence));
			break;
		case ICMP_DEST_UNREACH:
			printf("%sunreach:\n", prefix);
			break;
		case ICMP_SOURCE_QUENCH:
			printf("%ssource quench\n", prefix);
			break;
		case ICMP_REDIRECT:
			printf("%sredirect:\n", prefix);
			break;
		case ICMP_TIME_EXCEEDED:
			printf("%stime exceeded:\n", prefix);
			break;
		case ICMP_PARAMETERPROB:
			printf("%sparameter problem:\n", prefix);
			break;
		case ICMP_TIMESTAMP:
			printf("%stimestamp:\n", prefix);
			break;
		case ICMP_INFO_REQUEST:
			printf("%srequest:\n", prefix);
			break;
		case ICMP_INFO_REPLY:
			printf("%sreply:\n", prefix);
			break;
		case ICMP_ADDRESS:
			printf("%saddress:\n", prefix);
			break;
		case ICMP_ADDRESSREPLY:
			printf("%saddress reply:\n", prefix);
			break;
		default:
			break;
	}

	return 0;
}



