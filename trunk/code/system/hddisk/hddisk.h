/*
 *	@file	hddisk.h
 *
 *	@brief	declare all APIs in hddisk dir, include disk APIs, format APIs, fdisk APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2008-10-07
 */

#ifndef FZ_HDDISK_H
#define FZ_HDDISK_H

#include <sys/types.h>

/**
 *	Hard disk partition describe. the index means the 
 *	partition index(1-4), the begin is the begin sector, and
 *	the size the sector number in partition.
 */
typedef struct hddisk_part {
	int		active; /* this part is active or not */
	int		sysid;	/* system id, linux is 0x83 */
	u_int32_t	begin;	/* the begin sector */
	u_int32_t	size;	/* number of sector in partitions, 
				 * it support 2T size disk 
				 */
} hddisk_part_t;


/**
 *	Hard disk partition table, it only support 4 primary partitions
 *	in a hard disk.
 */
typedef struct hddisk_partbl {
	int		size;
	hddisk_part_t	tbl[4];
} hddisk_partbl_t;


/**
 *	Hard disk geometry, it's only used in old hard disk which size 
 *	is less 8.4G, and window/DOS will used it to load the OS. linux
 *	don't care about it.
 */
typedef struct hddisk_geometry {
	u_int32_t	ncyls;
	u_int32_t	nheads;
	u_int32_t	nsectors;
	u_int32_t	start;
} hddisk_geometry_t;


/**
 *	Check disk @disk is a valid disk in system, it need /proc filesystem 
 *	mounted.
 *
 *	Return 1 if valid, 0 if invalid.
 */
extern int 
hddisk_valid(const char *disk);


/**
 *	Get the sector number of disk @disk, it need /proc filesystem mounted.
 *
 *	Return the sector number of disk if success, -1 on error.
 */
extern long 
hddisk_size(const char *disk);


/**
 *	mount a disk @disk to a mount pointer @mnt, it'll try use "ext2",
 *	"ext3", "reiserfs" type to mount.
 *
 *	Return 0 if mount success.
 */
extern int 
hddisk_mount(const char *disk, const char *mnt);

/**
 *	umount a disk @disk if it's mount to a pointer, it need "/proc" is mounted.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
hddisk_umount(const char *disk);

/**
 *	Check disk @disk is mounted or not. it need "/proc" is mounted
 *
 *	Return 1 if mounted , 0 is not mounted.
 */
extern int 
hddisk_is_mounted(const char *disk);

/**
 *	Fdisk a disk according partition table @part, it can only fdisk a disk
 *	into no more than 4 main partitions, it didn't support logical partition.
 *	If the @part is NULL, the fdisk the whole disk one partitions.
 *
 *	Return 0 if fdisk success, -1 on error.
 */
extern int 
hddisk_set_partitions(const char *dev, hddisk_partbl_t *part);

/**
 *	Get the partition table from MBR of disk @disk.
 *
 *	Return 0 if success, -1 on error.
 */
extern int
hddisk_get_partitions(const char *dev, hddisk_partbl_t *part);

/**
 *	Clear all data in disk @disk. If the disk is whole disk, then the 
 *	partition table will cleared, all bytes in disk is zero. 
 *	
 *	Return 0 if success, -1 on error.
 */
extern int
hddisk_clear(const char *dev);

/**
 *	Get the geometry of a hard disk @disk, and stored the geometry info 
 *	into @geo.
 *
 *	Return 0 if success, -1 on error.
 */
extern int
hddisk_get_geometry(const char *dev, hddisk_geometry_t *geo);

/**
 *	create a ext3 filesystem on disk @disk. it use @tool as format
 *	tool, example "/sbin/mkfs.ext2".
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
hddisk_mke3fs(const char *disk);



#endif /* end of FZ_HDDISK_H  */

