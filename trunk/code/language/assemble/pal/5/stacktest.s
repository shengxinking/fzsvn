/* stacktest.s: test stack instruct */

	.section .data
value:
	.int 1, 2, 3, 4, 5

	.section .text
	.global _start

_start:
	nop

	movl $value, %esi
	pushl $1
	pushw $11
	popw %ax
	pushl $44
	popl value
	pushw $33
	popw 4(%esi)
	pushl value
	movl $1, %eax
	movl $2, %ebx
	movl $3, %ecx
	movl $4, %edx
	movl $5, %esi
	movl $6, %edi
	movl $7, %ebp

	pusha
	popa


	pushf
	popf

_exit:
	movl $0, %ebx
	movl $1, %eax
	int $0x80
	