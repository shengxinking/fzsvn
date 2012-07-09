/*
 *
 *
 *
 */

.section .data
value:
	.int 1234

.section .text
.globl _start

_start:
	nop
	movl value, %ecx
	movl $4321, %eax
	movl %eax, value
	movl $3421, value
	movl %eax, %ecx

_exit:	
	movl $1, %eax
	movl $0, %ebx
	int $0x80
	