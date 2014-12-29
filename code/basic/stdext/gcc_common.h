/**
 *	@file	gcc_common.h
 *
 *	@brief	the GCC common macro used for code.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2014-03-03
 */

#ifndef FZ_GCC_COMMON_H
#define FZ_GCC_COMMON_H

/* likely()/unlikely() for performance */
#if !defined(likely)
#if __GNUC__ < 3
#define __builtin_expect(x,y) (x)
#define likely(x) (x)
#define unlikely(x) (x)
#elif __GNUC__ < 4
/* gcc 3.x does the best job at this */
#define likely(x) (__builtin_expect((x) != 0, 1))
#define unlikely(x) (__builtin_expect((x) != 0, 0))
#else
/* GCC 4.x is stupid, it performs the comparison then compares it to 1,
 * so we cheat in a dirty way to prevent it from doing this. This will
 * only work with ints and booleans though.
 */
#define likely(x) (x)
#define unlikely(x) (__builtin_expect((unsigned long)(x), 0))
#endif
#endif

/* reduce CPU power */
#define	cpu_pause(void)	(asm volatile("rep:nop":::"memory"))

/* define data/pointer align for number and pointer */
#if !defined(align_num)
#ifdef	__i386__
#define	align_num(n)	((((n) + 3) / 4) * 4)
#define	align_ptr(p)	(void *)(((char *)(p) + 3) & ~(3))
#else
#define	align_num(n)	((((n) + 7) / 8) * 8)
#define	align_ptr(p)	(void *)(((unsigned long)(p) + 7) & (~(7)))
#endif
#endif	/* end of align_num */


#endif /* end of FZ_GCC_COMMON_H  */


