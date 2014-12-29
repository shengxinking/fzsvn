/* sizetest3.s: test BSS size */

	.section .data
	.fill 1000

	.section .text
	.globl _start
	
_start:
	movl $0, %ebx
	movl $1, %eax
	int $0x80
	