	.file	"packed.c"
.globl var1
	.data
	.align 4
	.type	var1, @object
	.size	var1, 16
var1:
	.byte	97
	.zero	3
	.long	1
	.value	1
	.byte	97
	.zero	1
	.value	1
	.byte	97
	.zero	1
.globl var2
	.type	var2, @object
	.size	var2, 11
var2:
	.byte	98
	.long	2
	.value	2
	.byte	98
	.value	2
	.byte	98
	.section	.rodata
.LC0:
	.string	"size of non-packed is %d\n"
.LC1:
	.string	"size of packed is %d\n"
	.text
.globl main
	.type	main, @function
main:
	leal	4(%esp), %ecx
	andl	$-16, %esp
	pushl	-4(%ecx)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ecx
	subl	$36, %esp
	movl	$0, -8(%ebp)
	movl	$16, 4(%esp)
	movl	$.LC0, (%esp)
	call	printf
	movl	$11, 4(%esp)
	movl	$.LC1, (%esp)
	call	printf
	movl	-8(%ebp), %eax
	addl	$36, %esp
	popl	%ecx
	popl	%ebp
	leal	-4(%ecx), %esp
	ret
	.size	main, .-main
	.ident	"GCC: (GNU) 4.1.0"
	.section	.note.GNU-stack,"",@progbits
