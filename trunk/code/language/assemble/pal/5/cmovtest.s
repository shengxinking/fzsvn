/*
 *
 *
 *
 */

.section .data
output:
	.asciz "The largest value is %d\n"
value:
	.int 105, 235, 62, 325, 134, 221, 53, 445, 117, 5

.section .text
.globl _start

_start:
	nop
	movl value, %ebx
	movl $1, %edi

loop:	
	movl value(, %edi, 4), %eax
	cmp %ebx, %eax
	cmova %eax, %ebx
	inc %edi
	cmp $10, %edi
	jne loop

	pushl %ebx
	pushl $output
	call printf

	addl $8, %esp
	pushl $0
	call exit

	