# hello.s

.data			# data section
	msg : .string "Hello, world!\n"
	len = . - msg

.text			# text section
.global _start		# entry function


_start:			# start function
	nop
	movl $len, %edx
	movl $msg, %ecx
	movl $1, %ebx
	movl $4, %eax
	int $0x80
	
exit:	
	movl $0, %ebx
	movl $1, %eax
	int $0x80


