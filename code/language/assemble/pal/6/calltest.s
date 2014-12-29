/* calltest.s: test call/ret instruct */

	.section .data
output:
	.asciz "This is section %d\n"

	
	
	.section .text
	.global _start

_start:
	nop

	pushl $1
	pushl $output
	call printf
	addl $8, %esp

	call func1

	pushl $3
	pushl $output
	call printf
	addl $8, %esp

	movl $0, %ebx
	movl $1, %eax
	int $0x80

	.type func1, @function
func1:
	pushl %ebp
	movl %esp, %ebp
	
	pushl $2
	pushl $output
	call printf
	addl $8, %esp
	
	movl %ebp, %esp
	popl %ebp
	ret

	