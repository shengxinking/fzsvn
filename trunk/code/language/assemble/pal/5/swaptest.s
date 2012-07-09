/* swaptest.s */

	.section .text

	.global _start

_start:
	nop
	movl $12345678, %eax
	bswap %eax

_exit:
	movl $0, %ebx
	movl $1, %eax
	int $0x80
	