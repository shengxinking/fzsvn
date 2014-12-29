	;; file: test1.asm

	section data
	i dq	0x01, 0x02, 0x03, 0x04
	j db	0x04, 0x03, 0x02


	section data1
x:	dq	0x55
y:	dw	0x11, 0x22
	
	section code
	global _start

_start:
	mov ax, 2
	mov [i], al
	mov [j], eax
	mov word [x], 4
	mov word [y], 5
	mov edx, [i]
	mov eax, [j]


_exit:
	mov ebx, 0
	mov eax, 4
	int 0x80

	