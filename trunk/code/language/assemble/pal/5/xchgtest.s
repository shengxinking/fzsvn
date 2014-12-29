/* xchgtest.s: test XCHG instruct */

	.section .data
value1:
	.int 0x11223344

	.section .text
	.global _start

_start:
	nop
	movl $0x55667788, %eax
	movl $0x12345678, %ebx
	xchg %eax, %ebx
	xchg %eax, value1

_exit:
	movl $0, %ebx
	movl $1, %eax
	int $0x80
	