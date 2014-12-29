/*
 *
 *
 *
 */

	.section .bss
	.lcomm buffer 10

	.section .data
str:
	.asciz	"hello world"
value:
	.int 0x12344321
value1:
	.short 0xaa55
value2:
	.byte 0xff
array:
	.int 1, 2, 3, 4, 5, 6

	.equ factor, 3
	.equ syscall, 0x80
	
	.section .rodata
	.asciz	"nihao"

	.section .text

	.globl _start
_start:
	nop
	// instant to register
	movl $1, %eax
	movw $0x80, %bx
	movb $0x55, %bl

	// instant to memory
	movl $1, buffer
	movw $0x80, str
	movb $96, str

	// register to register
	movl %eax, %ebx
	movw %cx, %dx
	movb %al, %dl

	// mem to register
	movl value, %ecx
	movw value1, %cx
	movb value2, %cl

	// register to mem
	movl %eax, value
	movw %bx, value1
	movb %cl, value2

	// base_address(offset_address, index, size)
	// equal: base_address + offset_address + index * size
	movl $18, %eax
	movl $5, %ecx
	movl %eax, array(, %ecx, 4)

	// register address
	mov $value, %edi
//	mov $0xfeef, (%edi)
	
	movl $1, %eax
	movl $0, %ebx
	int $0x80

	
