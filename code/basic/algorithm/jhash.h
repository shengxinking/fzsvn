/**
 *	@file	jhash.h
 *
 *	@brief	Linux kernel Jhash implement(include/linux/jhash.h)
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_JHASH_H
#define FZ_JHASH_H

#include <sys/types.h>

#include "gcc_common.h"

/* An arbitrary initial parameter */
#define	JHASH_INITVAL		0xdeadbeef

#define	JHASH_FINAL(a, b, c)			\
{						\
	a -= c;  a ^= rol32(c, 4);  c += b;	\
	b -= a;  b ^= rol32(a, 6);  a += c;	\
	c -= b;  c ^= rol32(b, 8);  b += a;	\
	a -= c;  a ^= rol32(c, 16); c += b;	\
	b -= a;  b ^= rol32(a, 19); a += c;	\
	c -= b;  c ^= rol32(b, 4);  b += a;	\
}


static inline u_int32_t  
jhash_3words(u_int32_t a, u_int32_t b, u_int32_t c)
{
	a += JHASH_INITVAL;
	b += JHASH_INITVAL;
	c += JHASH_INITVAL;

	JHASH_FINAL(a, b, c);

	return c;
}



#endif /* end of FZ_JHASH_H */


