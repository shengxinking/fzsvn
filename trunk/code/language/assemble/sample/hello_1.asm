	;; hello.asm

	;; data segment
	section data
	msg db "Hello, world!", 0x0A
	align 4
	len dd 0xf
	
	;;  code segment
	section code
	
	global _start
	
_start:
	mov ebx, len
	mov edx, [ebx]
	mov ecx, msg
	mov ebx, 1
	mov eax, 4
	int 0x80
	
_exit:
	mov ebx, 0
	mov eax, 1
	int 0x80

