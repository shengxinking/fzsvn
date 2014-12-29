/* test CMPXCHG instruct */

	.section .data
value1:
	.int 0x11223344

value2:
	.int 0x22334455

	.section .text
	.global _start

_start:
	nop
	movl value2, %ebx
	movl value1, %eax
	cmpxchg %ebx, value1
	
	movl %eax, value1
	movl $1234, %ebx
	cmpxchg %ebx, value2

	movl value1, %eax
	movl value2, %ebx
	movl value1, %ecx
	movl $2, %edx
	cmpxchg %ebx, %ecx

	movl value1, %eax
	movl value2, %ebx
	movl value1, %ecx
	movl $2, %edx
	cmpxchg %ebx, %edx


_exit:
	movl $0, %ebx
	movl $1, %eax
	int $0x80
	