/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

volatile int val = 1024;

static inline void 
clflush(volatile void *p)
{
	asm volatile("clflush (%0)" ::"r"(p));
}

static inline u_int64_t 
rdtsc(void)
{
	u_int64_t a, d;

	asm volatile ("cpuid; rdtsc" : "=a" (a), "=d" (d));
	//asm volatile ("rdtsc" : "=a" (a), "=d" (d));

	return a | ((u_int64_t)d << 32);
}

static inline void
test(void)
{
	u_int64_t start, end;

	start = rdtsc();
	val++;
	end = rdtsc();

	printf("took %lu ticks\n", end - start);
}

int 
main(void)
{
	test();
	test();
	printf("flush: ");
	clflush(&val);
	test();
	test();

	return 0;
}



