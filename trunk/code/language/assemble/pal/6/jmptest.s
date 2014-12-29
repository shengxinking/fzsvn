/*
 * jmptest.s: test jmp instruct
 *
 */


	.section .text
	.global _start

_start:
	nop
	movl $1, %eax
	jmp overhere
	movl $10, %ebx
	int $0x80

overhere:
	movl $100, %ebx
	int $0x80
	