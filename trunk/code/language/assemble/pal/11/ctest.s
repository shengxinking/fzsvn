	.file	"ctest.c"
	.section	.rodata
	.align 8
.LC0:
	.long	1293080650
	.long	1074340347
	.text
.globl function1
	.type	function1, @function
function1:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	imull	8(%ebp), %eax
	pushl	%eax
	fildl	(%esp)
	leal	4(%esp), %esp
	fldl	.LC0
	fmulp	%st, %st(1)
	fstps	-4(%ebp)
	movl	-4(%ebp), %eax
	movl	%eax, -24(%ebp)
	flds	-24(%ebp)
	leave
	ret
	.size	function1, .-function1
	.section	.rodata
.LC2:
	.string	"Radius: %d, Area: %f\n"
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
	movl	$10, -12(%ebp)
	movl	-12(%ebp), %eax
	movl	%eax, (%esp)
	call	function1
	fstps	-8(%ebp)
	flds	-8(%ebp)
	fstpl	8(%esp)
	movl	-12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	$.LC2, (%esp)
	call	printf
	movl	$2, -12(%ebp)
	movl	-12(%ebp), %eax
	movl	%eax, (%esp)
	call	function1
	fstps	-8(%ebp)
	flds	-8(%ebp)
	fstpl	8(%esp)
	movl	-12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	$.LC2, (%esp)
	call	printf
	movl	$120, -12(%ebp)
	movl	-12(%ebp), %eax
	movl	%eax, (%esp)
	call	function1
	fstps	-8(%ebp)
	flds	-8(%ebp)
	fstpl	8(%esp)
	movl	-12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	$.LC2, (%esp)
	call	printf
	movl	$0, %eax
	addl	$36, %esp
	popl	%ecx
	popl	%ebp
	leal	-4(%ecx), %esp
	ret
	.size	main, .-main
	.ident	"GCC: (GNU) 4.1.1"
	.section	.note.GNU-stack,"",@progbits
