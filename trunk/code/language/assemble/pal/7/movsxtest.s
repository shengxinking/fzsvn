/* movsxtest.s: test MOVSX instruct */

	.section .data
val1:	
	.int 0xf2345678
val2:
	.short -0x2222
val3:
	.byte 0x70

	.section .text
	.globl _start

_start:
	nop
	movsx val1, %eax
	movsx val2, %ebx
	movsx val3, %cx

	movw $-1122, %ax
	movsx %ax, %ebx
	movw $3344, %dx
	movsx %dx, %ecx
	movb $-33, %al
	movsx %al, %dx
	
	movl $1, %eax
	movl $0, %ebx
	int $0x80
	