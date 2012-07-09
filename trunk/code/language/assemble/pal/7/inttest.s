/* inttest.s : test signed int and unsigned int */

	.section .data
data:
	.int -45, 55

	.section .text
	.global _start

_start:
	nop
	movl $-345, %ecx
	movw $0xffb1, %dx
	movl data, %ebx
	movl $1, %eax
	int $0x80
	