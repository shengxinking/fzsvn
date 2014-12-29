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

int main(void)
{
	int *iptr;
	char *cptr;

	/* enable alignment checking on X86: set AC(18) in EFLAGS */
#if	defined	(__i386__)
	__asm__("pushf\n"
		"orl $0x40000, (%esp)\n"
		"popf");
	/* enable alignment checking on X86_64: set AC(18) in EFLAGS */
#elif	defined (__x86_64__)
	__asm__("pushf\n"
		"orl $0x40000, (%rsp)\n"
		"popf");
#else
#error	"invalid CPU type, only support i386/x86_64\n"
#endif

	cptr = (char *)malloc(sizeof(int) + 1);
	iptr = (int *)++cptr;
	
	*iptr = 42;

	return 0;
}


