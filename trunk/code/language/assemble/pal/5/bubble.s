/* bubble.s: bubble sort */

	.section .data
arr1:
	.int 105, 10, 23, 4, 9, 18, 122, 44, 75, 81, 99

	.section .text
	.global _start

_start:
	nop

	movl $arr1, %esi
	movl $10, %ecx
	movl $10, %ebx

loop:
	movl (%esi), %eax
	cmp %eax, 4(%esi)
	jge skip
	xchg %eax, 4(%esi)
	movl %eax, (%esi)

skip:
	addl $4, %esi
	dec %ebx
	jnz loop
	
	dec %ecx
	jz end

	movl $arr1, %esi
	movl %ecx, %ebx
	jmp loop

end:
	movl $1, %eax
	movl $0, %ebx
	int $0x80
	
	