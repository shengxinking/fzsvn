/* mmxtest.s: test MMX instruct */
	.section .data
val1:
	.quad -334455
val2:
	.int 1, -1
val3:
	.short 3, -3, 4, -4
val4:
	.byte 0x10, 0x05, 0xff, 0x32, 0x47, 0xe4, 0x00, 0x01

	.section .text
	.globl _start

_start:
	nop
	movq val1, %mm0
	movq val2, %mm2
	movq val3, %mm4
	movq val4, %mm7

	movl $0, %ebx
	movl $1, %eax
	int $0x80
	