/*
 *	@file	stage1.s
 *	@brief	read 512 bytes stage2 from sector 2 to memory 
 *
 *	@author	Forrest.zhang
 */
	
	
#include <first.h>

/**
 *	Return the Absolute address of give variable, first.exec is
 *	is start at 0x7c00
 */
#define FT_ABS(x)		(x-_start + 0x7c00)

/**
 *	Print a message on console.
 */
#define FT_MSG(x)		movw $FT_ABS(x), %si ; call message


/**
 *	Move memory content to register AL, it's a machine code.
 */
#define FT_MOV_MEM_TO_AL(x)	.byte 0xa0; .word x

	.file	"first.S"
	.text
	.code16

	.global _start

_start:
	/* jump to the real start place */
	jmp	ft_start
	nop
	. = _start + 4

/**
 *	This space is for the BIOS parameter block(BPB), it's used
 *	for reading Second stage using LBA mode(int 0x13 ah 0x42)  
 */
mode:
	.byte	0
disk_address_packet:
sectors:
	.long	0
heads:
	.long	0
cylinders:
	.word	0
sector_start:
	.byte	0
cylinder_start:
	.word	0

	. = _start + FT_BPB_OFFSET

/**
 *	Define some variables used in first.S
 */
ft_version:	
	.byte	FT_VER_MINOR, FT_VER_MAJOR
/*
ft_hddrive:
	.byte	0
ft_hdmode:
	.byte	0
*/
sd_address:
	.word	0x8000
sd_sector:
	.long	1
sd_segment:
	.word	0x800
ft_message:	
	.string	FT_MESSAGE

	. = _start + FT_SER_OFFSET
device_serial:
	.fill	20
	
/* the stage1 code start here */

ft_start:

	FT_MSG(ft_message)
	
	/**
	 * we have not stack, so interrupt is not safe here, we need 
	 * first close interrupt, them set %sp, and open interrupt.
	 */
	cli

	/* clear register ax, ds, ss */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %ss
	
	/* set stack segment as FT_STACKSEG */
	movw	$FT_STACKSEG, %sp
	
	sti

	/* save driver reference */
	pushw %dx

	jmp lba_mode

/**
 *	using LBA mode to read Second image, and store it to 
 */
lba_mode:

	movl 0x10(%si), %ecx
	movw $FT_ABS(disk_address_packet), %si
	movb $1, -1(%si)
	movl FT_ABS(sd_sector), %ebx
	
	/* set DAP parameters, it's used in int 0x13 call */
	movw $0x0010, (%si)	/* the DAP size(byte) and reserved byte */
	movw $1, 2(%si)		/* the block count need transfer(WORD) */
	movw $FT_BUFSEG, 6(%si)	/* offset for store data segment:offset */
	movl %ebx, 8(%si)	/* the lower DWORD start sector for transfer */
	xorl %eax, %eax
	movw %ax, 4(%si)	/* segment for store data(zero) */
	movw %ax, 12(%si)	/* the higher DWORD of start sector for transfer*/

	/* call INT 0x13 function AH 0x42 to read sector to FT_BUFSEG */
	movb $0x42, %ah
	int $0x13

	movw $FT_BUFSEG, %bx
	jmp copy_buffer

/**
 *	After read Second image from second sector from disk to FT_BUFSEG,
 *	we need copy Second image from address FT_BUFSEG to address SD_SEG
 */
copy_buffer:

	movw FT_ABS(sd_segment), %es

	/* copy Second stage from FT_BUFSEG to sd_segment */
	pusha
	pushw %ds

	movw $0x100, %cx	/* counter is 256 */
	movw %bx, %ds
	xorw %si, %si
	xorw %di, %di

	cld

	rep
	movsw

	popw %ds
	popa

	/* boot Second stage */
	jmp *(sd_address)

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

	/* Print CR and LF on screen */
	movw $0x0e0d, %ax
	int $0x10
	movb $0x0a, %al
	int $0x10
	
	ret


	. = _start + FT_PTT_OFFSET
	.fill 64
	
	/* the last two bytes in sector 0 must contain ST1_SIGNATURE
	 */
	. = _start + FT_SIG_OFFSET
	.word	FT_SIGNATURE
