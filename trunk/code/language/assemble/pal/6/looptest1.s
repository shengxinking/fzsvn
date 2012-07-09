/* looptest1.s: test LOOP instruct */

	.section .data
output:
	.asciz "The value is: %d\n"

	.section .text
	.globl _start

_start:
	nop
	movl $100, %ecx
	movl $0, %eax
	jcxz done

loop1:
	addl %ecx, %eax
	loop loop1

done:	
	pushl %eax
	pushl $output
	call printf
	addl $8, %esp

exit:	
	movl $1, %eax
	movl $0, %ebx
	int $0x80