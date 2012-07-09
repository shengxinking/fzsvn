/* cmptest.s: test cmp instruct */

	.section .data
output1:
	.asciz "%d is greate than %d\n"
output2:
	.asciz "%u is greate than %u\n"
	
	.section .text
	.global _start

_start:
	nop
	movl $15,  %eax
	movl $-10, %ebx
	cmp %ebx, %eax
	jg _sgreate
_sless:
	pushl %eax
	pushl %ebx
	pushl $output1
	call printf
	addl $12, %esp

	jmp _ucmp

_sgreate:
	pushl %ebx
	pushl %eax
	pushl $output1
	call printf
	addl $12, %esp

_ucmp:	
	movl $15, %eax
	movl $-10, %ebx
	cmp %ebx, %eax
	ja _ugreate

_uless:
	pushl %eax
	pushl %ebx
	pushl $output2
	call printf
	addl $12, %esp

	jmp _exit

_ugreate:
	pushl %ebx
	pushl %eax
	pushl $output2
	call printf
	addl $12, %esp

_exit:
	movl $1, %eax
	movl $0, %ebx
	int $0x80

	