/* paritytest.s: test PF bit */

	.section .text
	.globl _start

_start:
	nop
	movl $1, %eax

	movl $4, %ebx
	subl $1, %ebx
	jp overhere
	int $0x80

overhere:
	movl $100, %ebx
	int $0x80
	