/* movztest.s: test MOVZ instruct */

	.section .data
val1:	
	.int 0x12345678
val2:
	.short 0x2222
val3:
	.byte 0xf0

	.section .text
	.globl _start

_start:
	nop
	movzx val1, %eax
	movzx val2, %ebx
	movzx val3, %cx

	movw $1122, %ax
	movzx %ax, %ebx
	movw $3344, %dx
	movzx %dx, %ecx

	movl $1, %eax
	movl $0, %ebx
	int $0x80
	