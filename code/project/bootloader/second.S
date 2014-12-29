/**
 *	file	second.S
 *	brief	The second stage of bootload, it read all left bootcode from 3rd sector
 *		and store it in memory, and jump to the bl_main entry, execute the C
 *		code.
 *
 */

//#include <second_size.h>
	
#define	SD_ABS(x)	(x - _start + 0x8000)
#define SD_MSG(x)	movw $SD_ABS(x), %si ; call message

	.file "second.S"
	.text
	.code16
	.global _start

_start:

	jmp sd_start
	. = _start + 4
	
/**
 *	Define some variable used for second.S
 */
sd_message:
	.string	"Second start..."
	

sd_start:	
	
	nop
	SD_MSG(sd_message)

	pushw %dx
	pushw %si

	popw %si

	nop

/**
 *	Show a message on console, it using BIOS int 10H to print message,
 *	AH is 0xe, it print a char in screen and Cursor will move to next
 *	char.
 */
1:
	movw $0x0001, %bx	/* bh = 0, bl = 1 */
	movb $0xe, %ah
	int $0x10
	
message:
	lodsb
	cmpb $0, %al
	jne 1b
	ret

