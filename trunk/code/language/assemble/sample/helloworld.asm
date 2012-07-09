	;;  helloworld.asm

	;; data section
	section .data
	msg db "Hello, world!", 0xA
	len equ $ - msg


	;; text section
	section .text
	global _start

_start:
	mov edx, len
	mov ecx, msg
	mov eax, 4
	int 0x80

_exit:	
	mov ebx, 0
	mov eax, 1
	int 0x80
	