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
#include <string.h>
#include <stdio.h>


int main(void)
{
	int a = 3, b = 5;

	printf("a is %d, b is %d\n", a, b);

	__asm__ __volatile__ (

		"movl %2, %%ecx \n"
		"movl %3, %%edx \n"
		"movl %%ecx, %1 \n"
		"movl %%edx, %0 \n"
		: "=m"(a), "=m"(b)
		: "r"(7), "r"(8)
		: "%ecx", "%edx"
		); 

	printf("after swap: a is %d, b is %d\n", a, b);

	return 0;

}

