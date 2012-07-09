/*
 *	@file	checksum.h
 *
 *	@brief	Declare IP checksum, TCP checksum, UDP checksum, ICMP checksum.
 *	
 *	@date	2008-07-18
 */

#ifndef FZ_CHECKSUM_H
#define FZ_CHECKSUM_H

#include <sys/types.h>

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
extern u_int16_t 
csum_ip(const u_int16_t * w, int blen );

/**
 *	Calculate TCP checksum, it can generate TCP checksum or check the TCP 
 *	checksum is valid or not. The pseudo header is in @h, it's length is 12
 *	The TCP header and payload is in @d, it's length is @dlen.
 *	If it's generate TCP checksum, the TCPHdr->checksum need be zero. 
 *
 *	Return the checksum of TCP.
 */
extern u_int16_t 
csum_tcp(const u_int16_t *h, const u_int16_t *d, int dlen );


#endif /* end of FZ_CHECKSUM_H  */

