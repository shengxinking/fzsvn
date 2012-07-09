/*
 *	@file	checksum.c	
 *
 *	@brief	Define checksum function for IP, TCP, UDP, ICMP, IP6, ICMP6.
 *	
 *	@date	2008-07-18
 */

#include <unistd.h>
#include <stdlib.h>

#include "checksum.h"

/**
 *	Calculate IP checksum, it can be used generate IP checksum or check 
 *	the IP checksum, the IP packet header is begin at @w, IP header length
 *	is @blen. The IP checksum algorithm is:
 *	
 *	Generate:
 *		1. Before call this function, set IPHdr->csum = 0.
 *		2. Look IP Header as u_int16_t array, add all element in array into @sum.
 *		3. The complement of @sum is the checksum.
 *
 *	Validate:
 *		1. Look IP Header as u_int16_t array, add all element in array into @sum.
 *		2. Let @sum equal as the complement of @sum.
 *		3. If the @sum is zero, the IP checksum is valid, or else is invalid.
 *
 *	Return the checksum of IP header.
 */
u_int16_t 
csum_ip(const u_int16_t *w, int blen)
{
	u_int32_t cksum;

	/* IP must be >= 20 bytes */
	cksum  = w[0];
	cksum += w[1];
	cksum += w[2];
	cksum += w[3];
	cksum += w[4];
	cksum += w[5];
	cksum += w[6];
	cksum += w[7];
	cksum += w[8];
	cksum += w[9];

	blen  -= 20;
	w     += 10;

	/* IP header must be 4* bytes */
	while (blen) {
		cksum += w[0];
		cksum += w[1];
		w     += 2;
		blen  -= 4;
	}

	cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
	cksum += (cksum >> 16);
 
	return (u_int16_t) (~cksum);
}

/**
 *	Calculate TCP checksum, it can generate TCP checksum or check the TCP 
 *	checksum is valid or not. The pseudo header is in @h, it's length is 12
 *	The TCP header and payload is in @d, it's length is @dlen.
 *
 *
 *	Return the checksum of TCP.
 */
u_int16_t 
csum_tcp(const u_int16_t *h, const u_int16_t *d, int dlen)
{
	u_int32_t cksum;
	u_int16_t answer=0;

	/* PseudoHeader must have 12 bytes */
	cksum  = h[0];
	cksum += h[1];
	cksum += h[2];
	cksum += h[3];
	cksum += h[4];
	cksum += h[5];

	/* TCP hdr must have 20 hdr bytes */
	cksum += d[0];
	cksum += d[1];
	cksum += d[2];
	cksum += d[3];
	cksum += d[4];
	cksum += d[5];
	cksum += d[6];
	cksum += d[7];
	cksum += d[8];
	cksum += d[9];

	dlen  -= 20; /* bytes   */
	d     += 10; /* short's */ 

	while (dlen >=32) {
		cksum += d[0];
		cksum += d[1];
		cksum += d[2];
		cksum += d[3];
		cksum += d[4];
		cksum += d[5];
		cksum += d[6];
		cksum += d[7];
		cksum += d[8];
		cksum += d[9];
		cksum += d[10];
		cksum += d[11];
		cksum += d[12];
		cksum += d[13];
		cksum += d[14];
		cksum += d[15];
		d     += 16;
		dlen  -= 32;
	}

	while (dlen >=8) {
		cksum += d[0];
		cksum += d[1];
		cksum += d[2];
		cksum += d[3];
		d     += 4;   
		dlen  -= 8;
	}

	while (dlen > 1) {
		cksum += *d++;
		dlen  -= 2;
	}

	if (dlen == 1) { 
		/* printf("new checksum odd byte-packet\n"); */
		*(unsigned char*)(&answer) = (*(unsigned char*)d);

		/* cksum += (u_int16_t) (*(u_int8_t*)d); */
     
		cksum += answer;
	}
   
	cksum  = (cksum >> 16) + (cksum & 0x0000ffff);
	cksum += (cksum >> 16);
 
	return (u_int16_t)(~cksum);
}











