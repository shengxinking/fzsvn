	.file	"asm_test3.c"
	.section	.rodata
.LC0:
	.string	"a is %d, b is %d\n"
.LC1:
	.string	"after swap: a is %d, b is %d\n"
	.text
.globl main
	.type	main, @function
main:
	leal	4(%esp), %ecx
	andl	$-16, %esp
	pushl	-4(%ecx)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%ecx
	subl	$32, %esp
	movl	$3, -12(%ebp)
	movl	$5, -16(%ebp)
	movl	-16(%ebp), %eax
	movl	-12(%ebp), %edx
	movl	%eax, 8(%esp)
	movl	%edx, 4(%esp)
	movl	$.LC0, (%esp)
	call	printf
	movl	$7, %ebx
	movl	$8, %eax
#APP
	movl %ebx, %ecx 
movl %eax, %edx 
movl %ecx, -16(%ebp) 
movl %edx, -12(%ebp) 

#NO_APP
	movl	-16(%ebp), %eax
	movl	-12(%ebp), %edx
	movl	%eax, 8(%esp)
	movl	%edx, 4(%esp)
	movl	$.LC1, (%esp)
	call	printf
	movl	$0, %eax
	addl	$32, %esp
	popl	%ecx
	popl	%ebx
	popl	%ebp
	leal	-4(%ecx), %esp
	ret
	.size	main, .-main
	.ident	"GCC: (GNU) 4.1.2 (Gentoo 4.1.2 p1.1)"
	.section	.note.GNU-stack,"",@progbits
