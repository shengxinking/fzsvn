/* xaddtest.s: test XADD instruct */

	.section .data
value1:
	.int 0x11223344

	.section .text
	.global _start

_start:
	nop
	movl $0x44332211, %eax
	xadd %eax, value1

	movl $0x11111111, %ebx
	xadd %ebx, %eax

_exit:
	movl $0, %ebx
	movl $1, %eax
	int $0x80

	