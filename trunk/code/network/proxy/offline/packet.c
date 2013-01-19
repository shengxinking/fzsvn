/**
 *	file	decode.c
 *	brief	Define a set of function to decode raw network packet, convert 
 *		it to Packet format, check the packet valid.
 *
 *	date	2008-07-18
 */


#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "packet.h"
#include "checksum.h"

/**
 *	Define macro to print error message
 */
#define _PACKET_DBG
#ifdef	_PACKET_DBG
#define _PACKET_ERR(fmt, args...)	printf("decode:%s:%d: " fmt, \
					       __FILE__, __LINE__, ##args)
#else
#define _PACKET_ERR(fmt, args...)
#endif


/**
 *	Decode Ethernet packet get from PCAP, the packet data store in @pkt, 
 *	PCAP header is @pkthdr.
 *
 *	Return 0 if success, -1 on error.
 */
int 
DecodeEthPkt(Packet *p, const struct pcap_pkthdr * pkthdr, const u_int8_t *pkt) 
{
	u_int32_t pkt_len;      /* suprisingly, the length of the packet */
	u_int32_t cap_len;      /* caplen value */
        
	bzero((char *) p, sizeof(Packet));

	p->pkth = pkthdr;
	p->pkt = pkt;

	/* set the lengths we need */
	pkt_len = pkthdr->len;  /* total packet length */
	cap_len = pkthdr->caplen;   /* captured packet length */

	if(pkt_len < cap_len) {
		_PACKET_ERR("Packet capture length is %u, capture length is %u\n",
			    pkt_len, cap_len);
		return -1;
	}
    

	/* do a little validation */
	if(cap_len < ETHERNET_HEADER_LEN) {
		_PACKET_ERR("Packet capture length is %u\n", cap_len);
		return -1;
	}

	/* lay the ethernet structure over the packet data */
	p->eh = (EtherHdr *) pkt;


	/* grab out the network type */
	switch(ntohs(p->eh->ether_type)) {
        
	case ETHERNET_TYPE_IP:
		
		return DecodeIP(p->pkt + ETHERNET_HEADER_LEN, 
			 cap_len - ETHERNET_HEADER_LEN, p);


	default:
		return -1;
	}
	
	return 0;
}


/**
 *	Test IP header to avoid some attack:
 *		Land Attack(same src/dst ip)
 *		Loopback (src or dst in 127/8 block)
 * 
 *	Return 0 if success, -1 on error
 */
static int 
_IPHdrTests(Packet *p) 
{
	assert(p);

	if( p->iph->ip_src.s_addr == p->iph->ip_dst.s_addr ) {
		_PACKET_ERR("IP header saddr is same as daddr\n");
		return -1;
	}

	if( (p->iph->ip_src.s_addr & 0xff) == 0x7f
	    || (p->iph->ip_dst.s_addr & 0xff) == 0x7f ) {
		_PACKET_ERR("IP header saddr||daddr is loopback\n");
		return -1;
        }

	return 0;
}


/**
 *	Decode IP packet, and store the results of decode to @p, the IP packet
 *	stored in @pkt, length is @len.
 *
 *	Return 0 if success, -1 on error.
 */
int 
DecodeIP(const u_int8_t *pkt, const u_int32_t len, Packet *p)
{
	u_int32_t ip_len; /* length from the start of the ip hdr to the pkt end */
	u_int32_t hlen;   /* ip header length */
	u_int16_t csum;   /* checksum */

	/* do a little validation */
	if(len < IP_HEADER_LEN) {
		_PACKET_ERR("packet length %d is less than IP header length\n", 
			    len);
		p->iph = NULL;
		return -1;
	}

	/* set the IP datagram length */
	ip_len = ntohs(p->iph->ip_len);

	/* set the IP header length */
	hlen = IP_HLEN(p->iph) << 2;

	/* IP header length sanity check */
	if(hlen < IP_HEADER_LEN) {
		_PACKET_ERR("IP header length %d is less than IP header length\n",
			    hlen);
		p->iph = NULL;
		return -1;
	}

	/* Check IP packet length sanity check */
	if (ip_len != len) {
		_PACKET_ERR("IP packet length different: %u/%u\n",
			    ip_len, len);
		p->iph = NULL;
		return -1;
	}

	/* 
	 * IP Header tests: Land attack, and Loop back test 
	 */
	if (_IPHdrTests(p))
		return -1;


	if(IP_CHECKSUMS) {
		csum = csum_ip((u_short *)p->iph, hlen);

		if(csum) {
			_PACKET_ERR("Bad IP checksum\n");
			p->iph = NULL;
			return -1;
		}
	}

	/* set the remaining packet length */
	ip_len -= hlen;

	/* check for fragmented packets */
	p->frag_offset = ntohs(p->iph->ip_off);

	/** 
	 * get the values of the reserved, more 
	 * fragments and don't fragment flags 
	 */
	p->rf = (u_int8_t)((p->frag_offset & 0x8000) >> 15);
	p->df = (u_int8_t)((p->frag_offset & 0x4000) >> 14);
	p->mf = (u_int8_t)((p->frag_offset & 0x2000) >> 13);

	/* mask off the high bits in the fragment offset field */
	p->frag_offset &= 0x1FFF;

	if (p->frag_offset || p->mf) {
		/* set the packet fragment flag */
		p->frag_flag = 1;
	} 
	else  {
		p->frag_flag = 0;
	}

	/* Set some convienience pointers */
	p->ip_data = pkt + hlen;
	p->ip_dsize = (u_short) ip_len;

	/* if this packet isn't a fragment
	 * or if it is, its a UDP packet and offset isn't 0 */
	if ( !(p->frag_flag) || 
	     (p->frag_flag && (p->frag_offset == 0) && 
	      (p->iph->ip_proto == IPPROTO_UDP)))
	{

		switch (p->iph->ip_proto) {

		case IPPROTO_TCP:
			return DecodeTCP(pkt + hlen, ip_len, p);

		default:
			return -1;
		}
	}
	else {
		/* set the payload pointer and payload size */
		p->data = pkt + hlen;
		p->dsize = (u_short) ip_len;
		return 0;
	}

	p->data = pkt + hlen;
	p->dsize = (u_short) ip_len;

	return 0;
}


/**
 *	Decode TCP packet and store the results in @p, the TCP packet
 *	is stored in @pkt, length is @len.
 *
 *	Return 0 if parse success, -1 on error.
 */
int 
DecodeTCP(const u_int8_t *pkt, const u_int32_t len, Packet *p)
{
	struct pseudoheader6       /* pseudo header for TCP checksum calculations */
	{
		u_int32_t sip[4], dip[4];   /* IP addr */
		u_int8_t  zero;       /* checksum placeholder */
		u_int8_t  protocol;   /* protocol number */
		u_int16_t tcplen;     /* tcp packet length */
	};

	struct pseudoheader       /* pseudo header for TCP checksum calculations */
	{
		u_int32_t sip, dip;   /* IP addr */
		u_int8_t  zero;       /* checksum placeholder */
		u_int8_t  protocol;   /* protocol number */
		u_int16_t tcplen;     /* tcp packet length */
	};
	u_int32_t hlen;            /* TCP header length */
	u_short csum;              /* checksum */
	struct pseudoheader ph;    /* pseudo header declaration */

	/* TCP packet length sanity check */
	if(len < 20) {
		_PACKET_ERR("TCP packet length %d is less than TCP header length\n",
			    len);
		p->tcph = NULL;

		return -1;
	}

	/* lay TCP on top of the data cause there is enough of it! */
	p->tcph = (TCPHdr *) pkt;

	/* multiply the payload offset value by 4 */
	hlen = TCP_OFFSET(p->tcph) << 2;

	if(hlen < 20) {
		p->tcph = NULL;
		return -1;
	}

	if(hlen > len) {
		_PACKET_ERR("TCP header length %u is greate than packet len\n",
			    hlen);
		p->tcph = NULL;
		return -1;
	}

	/* stuff more data into the printout data struct */
	p->sp = ntohs(p->tcph->th_sport);
	p->dp = ntohs(p->tcph->th_dport);

	if (TCP_CHECKSUMS) {
		ph.sip = (u_int32_t)(p->iph->ip_src.s_addr);
		ph.dip = (u_int32_t)(p->iph->ip_dst.s_addr);

		/* setup the pseudo header for checksum calculation */
		ph.zero = 0;
		ph.protocol = GET_IPH_PROTO(p);
		ph.tcplen = htons((u_short)len);
    
		/* if we're being "stateless" we probably don't care about the TCP 
		 * checksum, but it's not bad to keep around for shits and giggles */
		/* calculate the checksum */
		csum = csum_tcp((u_int16_t *)&ph, (u_int16_t *)(p->tcph), len);
		if(csum) {
			_PACKET_ERR("TCP checksum is invalid\n");
			return -1;
		}
	}


	/* set the data pointer and size */
	p->data = (u_int8_t *) (pkt + hlen);

	if(hlen < len) {
		p->dsize = (u_short)(len - hlen);
	}
	else {
		p->dsize = 0;
	}

	return 0;
}





