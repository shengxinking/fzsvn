/*
 *
 *
 *
 *
 */

	.section .data
precision:
	.byte 0x7f, 0x00

	.section .bss
	.lcomm result, 4

	.section .text
	.globl _start

_start:
	nop
	finit
	fldcw precision

	pushl $10
	call area
	addl $4, %esp

	pushl $2
	call area
	addl $4, %esp

	pushl $120
	call area
	addl $4, %esp

	movl $1, %eax
	movl $0, %ebx
	int $0x80

	