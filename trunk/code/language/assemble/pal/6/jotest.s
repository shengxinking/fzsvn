/* jotest.s: test jo instruct */

	.section .text
	.global _start

_start:
	nop
	movl $1, %eax
	movb $0xff, %bl
	addb $10, %bl
	jc overhere
	int $0x80

overhere:
	movl $0, %ebx
	int $0x80

	