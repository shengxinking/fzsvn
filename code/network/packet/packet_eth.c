/**
 *	@file	packet_eth.c
 *
 *	@brief	Ethernet packet decode/encode API.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include "gcc_common.h"
#include "dbg_common.h"

#include "packet_eth.h"


int 
eth_decode(netpkt_t *pkt)
{
	int n;
	u_int32_t vlan;
	u_int16_t type;

	if (unlikely(!pkt))
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(!pkt->head))
		ERR_RET(-1, "no data in packet\n");

	n = pkt->tail - pkt->head;
	if (unlikely(n < ETH_HLEN))
		ERR_RET(-1, "invalid ether header\n");

	/* get layer 2 header */
	pkt->eth = (struct ether_header *)pkt->head;
	pkt->hdr2_len = ETH_HLEN;

	/* get layer 3 type and set payload */
	pkt->hdr3_type = ntohs(pkt->eth->ether_type);

	/* vlan packet */
	if (pkt->hdr3_type == ETHERTYPE_VLAN) {
		vlan = ntohs(*(u_int16_t *)(pkt->head + ETH_HLEN));
		type = ntohs(*(u_int16_t *)(pkt->head + ETH_HLEN + 2));
		
		pkt->vlan_cfi = (vlan >> 12) & 0x1;
		pkt->vlan_pcp = (vlan >> 13) & 0x7;
		pkt->vlan_vid = vlan & 0xfff;

		/* recalc hdr3 type and length */
		pkt->hdr3_type = type;
		pkt->hdr2_len += 4;
	}

	return 0;
}

int 
eth_print(const netpkt_t *pkt, const char *prefix)
{
	int n;
	int m;
	struct ether_header *h;

	if (unlikely(!pkt) || !prefix)
		ERR_RET(-1, "invalid argument\n");

	if (unlikely(!pkt->eth))
		ERR_RET(-1, "not decode layer 2 header\n");
	
	h = pkt->eth;
	n = pkt->tail - pkt->head;
	m = pkt->hdr2_len;

	printf("%slen:            %d\n", prefix, pkt->hdr2_len);
	printf("%sdata:           %d\n", prefix, (n - m));
	printf("%sDMAC:           %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", 
		prefix, 
		h->ether_dhost[0],
		h->ether_dhost[1],
		h->ether_dhost[2],
		h->ether_dhost[3],
		h->ether_dhost[4],
		h->ether_dhost[5]);
	printf("%sSMAC:           %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", 
		prefix, 
		h->ether_shost[0],
		h->ether_shost[1],
		h->ether_shost[2],
		h->ether_shost[3],
		h->ether_shost[4],
		h->ether_shost[5]);
	printf("%stype:           %.4x\n", prefix, ntohs(h->ether_type));	

	/* vlan packet */
	if (pkt->vlan_vid) {
		printf("%svlan_pcp:\t%x\n",  prefix, pkt->vlan_pcp);
		printf("%svlan_cfi:\t%x\n",  prefix, pkt->vlan_cfi);
		printf("%svlan_vid:\t%x\n",  prefix, pkt->vlan_vid);
	}

	return 0;
}





