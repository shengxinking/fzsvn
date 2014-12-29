	;; args.asm	print all args in command line

	;; text section
	.text

	.global _start

_start:
	pop ecx

_next:	
	pop ecx
	test ecx, ecx
	jz exit
	mov ebx, ecx
	xor edx, edx

_strlen:
	mov al, ebx
	inc edx
	inc ebx
	