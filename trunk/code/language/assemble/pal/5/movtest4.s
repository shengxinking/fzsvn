/*
 *
 *
 *
 */

.section .data
value:
	.int 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60

.section .text

.globl _start

_start:
	nop
	movl value, %eax
	movl $value, %edi
	movl $100, 4(%edi)
	movl $1, %edi
	movl value(, %edi, 4), %ebx

_exit:
	movl $1, %eax
	int $0x80
	