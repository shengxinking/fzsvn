# cpuid .s


.data
	output : .string "The processor vendor ID is 'xxxxxxxxxxxx'\n"
	len = . - output

.text
.global _start


_start:
	movl $0, %eax
	cpuid
	movl $output, %edi
	movl %ebx, 28(%edi)
	movl %edx, 32(%edi)
	movl %ecx, 36(%edi)
	movl $4, %eax
	movl $1, %ebx
	movl $output, %ecx
	movl $len, %edx
	int  $0x80
	movl $1, %eax
	movl $0, %ebx
	int  $0x80

	