/*
 *	@file	mbr.h
 *	@brief	MBR manipulate functions
 * 
 *	@author	Forrest.zhang
 */

#ifndef FZ_MBR_H
#define FZ_MBR_H

#include <sys/types.h>

/**
 *	The patition of disk, in PC's MBR, there have 4 main partitions.
 */
typedef struct partition {
	u_int8_t	bootid;		/* bootable identifier: 0 nobootable,0x80 bootable */
	u_int8_t	beg_head;	/* begin of head */
	u_int8_t	beg_sect;	/* begin of sector */
	u_int8_t	beg_cyl;	/* begin of cylinder */
	u_int8_t	sysid;		/* system identifier: 83 mean Linux */
	u_int8_t	end_head;	/* end of head */
	u_int8_t	end_sect;	/* end of sector */
	u_int8_t	end_cyl;	/* end of cylinder */
	u_int32_t	start_sect;	/* start of real sector */
	u_int32_t	nr_sect;	/* number of sectors */
} partition_t;

/**
 *	The MBR structure, it's for PC.
 */
typedef struct mbr {
	u_int8_t	bootcode[446];	/* the actual boot code in PC MBR */
	partition_t     partitions[4];	/* 4 partitions in PC MBR */
	int		npartitions;	/* the number of real partitions in MBR */
	u_int16_t	signature;	/* 0xAA55 means PC MBR format */
} mbr_t;


/**
 *	Get a MBR from a hard disk device and stored in @mbr. 
 *	If the dev is not a block device, In linux, disk device is
 *	alway like "/dev/sd[a-z][1-N]", "/dev/sg[1-N]", "/dev/hd[a-z][1-N]" 
 *	return -1.
 *
 *	Return 0 if success, -1 on error.
 */
extern int mbr_get(const char *dev, mbr_t *mbr);

/**
 *	Print MBR partition table
 *
 *	No return
 */
extern void mbr_print(const mbr_t *mbr);


/**
 *	Write MBR to a device, it's always used to partition disk.
 *
 *	Return 0 if write success, -1 on error.
 */
extern int mbr_write(const char *dev, mbr_t *mbr);


#endif /* end of FZ_MBR_H */

