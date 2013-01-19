/**
 *	@file	chechsum.h 	
 *	@brief	A fast TCP, UDP, ICMP checksum function implement.
 *
 *	@author	Berg
 *
 */

#ifndef FZ_CHECKSUM_H
#define FZ_CHECKSUM_H

#include <sys/types.h>

/**
 *	Create a TCP/UDP checksum according source IP @saddr, dest IP @daddr,	
 *	protocol number @proto, total data length @data_len(network byte sequence), 
 *	and tcp/udp packet buffer @ptr, the TCP/UDP packet buffer length is @len(use bytes).
 *
 *	Return the checksum of give TCP/UDP packet.
 */
static inline u_int16_t 
csum_tcpudp(u_int32_t saddr, u_int32_t daddr, u_int8_t proto, u_int16_t data_len, 
	    u_int16_t *ptr, u_int16_t len)
{
	u_int16_t val;

	__asm__ __volatile__ (
	"	xorl %%eax, %%eax	\n\
		movl %%eax, %%ebx	\n\
		movl %%eax, %%ecx	\n\
		movl %%eax, %%edx	\n\
		pushl %1			\n\
		pushl %2			\n\
		movb %3, %%bh		\n\
		pushw %%bx			\n\
		pushw %4			\n\
		movb $6, %%cl		\n\
	1:	popw %%bx			\n\
		addl %%ebx, %%eax	\n\
		dec %%cl			\n\
		jnz 1b				\n\
		movl %5, %%ebx		\n\
		movw %6, %%cx		\n\
	2:	movw (%%ebx), %%dx	\n\
		addl %%edx, %%eax	\n\
		subw $2, %%cx		\n\
		addl $2, %%ebx		\n\
		cmp $1, %%cx		\n\
		jg 2b				\n\
		jl 3f				\n\
		xorl %%edx, %%edx	\n\
		movb (%%ebx), %%dl	\n\
		addl %%edx, %%eax	\n\
	3:	xorl %%ebx, %%ebx	\n\
		movw %%ax, %%bx		\n\
		shrl $16, %%eax		\n\
		addl %%ebx, %%eax	\n\
		movl %%eax, %%ebx	\n\
		shrl $16, %%ebx		\n\
		addl %%ebx, %%eax	\n\
		notl %%eax			\n\
		movw %%ax, %0;"
	:"=g"(val)
	:"g"(saddr), "g"(daddr), "g"(proto), "g"(data_len), "g"(ptr), "g"(len)
	:"%eax", "%ebx", "%ecx", "%edx"
	);

	return val;
}

static inline u_int16_t 
csum_icmp(u_int16_t *addr, u_int16_t len)
{
	u_int16_t val;

	__asm__ __volatile__ (
	"	xorl %%eax, %%eax	\n\
		movl %1, %%ebx		\n\
		movl %%eax, %%ecx	\n\
		movl %%eax, %%edx	\n\
		movw %2, %%cx		\n\
	1:	movw (%%ebx), %%dx	\n\
		addl %%edx, %%eax	\n\
		subw $2, %%cx		\n\
		addl $2, %%ebx		\n\
		cmp $1, %%cx		\n\
		jg 1b				\n\
		jl 2f				\n\
		xorl %%edx, %%edx	\n\
		movb (%%ebx), %%dl	\n\
		addl %%edx, %%eax	\n\
	2:	xorl %%ebx, %%ebx	\n\
		movw %%ax, %%bx		\n\
		shrl $16, %%eax		\n\
		addl %%ebx, %%eax	\n\
		movl %%eax, %%ebx	\n\
		shrl $16, %%ebx		\n\
		addl %%ebx, %%eax	\n\
		notl %%eax			\n\
		movw %%ax, %0;"
	:"=g"(val)
	:"g"(addr), "g"(len)
	:"%eax", "%ebx", "%ecx", "%edx"
	);

	return val;
}


#endif /* end of FZ_CHECKSUM_H */

