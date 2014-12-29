# args, handle command line parameter


.text
.globle _start


_start:
	popl %ecx        # argc is at top of stack

vnext:
	popl %ecx        # following argv
	test %ecs, %ecs  # test if argv[N] is NULL
	jz   exit
	movl %ecx, %ebx
	xorl %edx, %edx

strlen:
	movb (%ebx), %al
	inc %edx
	inc %ebx
	test %al, %al
	jnz strlen
	movb $10, -1(%ebx)
	movl $4, %eax
	movl $1, %ebx
	int $0x80
	jmp vnext

exit:
	movl $1, %eax
	xorl %ebx, %ebx
	int $0x80

	ret