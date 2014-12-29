/* signtest.s: test SF */

	.section .data
value:	
	.int 21, 15, 24, 33, 6, 8, 10, 2
output:
	.asciz "The value is: %d\n"

	.section .text
	.globl _start

_start:
	nop
	movl $7, %edi
loop:
	pushl value(, %edi, 4)
	pushl $output
	call printf
	addl $8, %esp
	dec %edi
	jns loop

	movl $1, %eax
	movl $0, %ebx
	int $0x80
	