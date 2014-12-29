/**
 *	@file	stage1.h
 *	@brief	define some MACROs for stage1.s
 *
 *	@author	Forrest.Zhang
 */

#ifndef FZ_FIRST1_H
#define FZ_FIRST1_H	1

/* define stage1 version */
#define FT_VER_MAJOR		0x00
#define FT_VER_MINOR		0x01

/* define some offset address */
#define FT_BPB_OFFSET		0x3e
#define FT_PTT_OFFSET		0x1be
#define FT_SIG_OFFSET		0x1fe

/* define some address */
#define FT_STACKSEG		0x2000

/* define memory address for store disk data */
#define FT_BUFSEG		0x7000

/* define serial number address */
#define FT_SER_OFFSET		0x60

/* end of MBR, must be 0xaa55 */
#define FT_SIGNATURE		0xaa55

#define FT_MESSAGE		"First start..."

#endif /* end of FZ_STAGE1_H */
