/* sizetest1.s: test BSS size */

	.section .text
	.globl _start
	
_start:
	movl $0, %ebx
	movl $1, %eax
	int $0x80
	
	