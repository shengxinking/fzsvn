/*
 *	@file	hddisk.c
 *
 *	@brief	defined some APIs to recognise hard disk, mount/umount 
 *		harddisk, partition harddisk, and format harddisk.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/hdreg.h>

#include "hddisk.h"

#define _HDBLOCK_SIZE	4096
#define _HDISK_PROC	"/proc/partitions"
#define _HDMOUNT_PROC	"/proc/mounts"

/**
 *	/proc/partitions line structure.
 */
typedef struct _diskproc {
	int		major;
	int		minor;
	u_int32_t	nblocks;
	char		name[64];
} _diskproc_t ;

/**
 *	/proc/mounts line structure.
 */
typedef struct _mountproc {
	char		dev[64];
	char		mnt[64];
	char		type[32];
	char		opt[128];
	int		dump;
	int		pass;
} _mountproc_t;

/**
 *	MBR partition table struct.
 */
typedef struct _mbrpart {
	u_int8_t	bootid;		/* active flag: 0x80 bootable */
	u_int8_t	beg_head;	/* begin of head, max 254 heads */
	u_int8_t	beg_sect;	/* low 6-bit is begin of sector, high 
					 * 2-bit is high of begin cylinder. 
					 * from 0-63.
					 */
	u_int8_t	beg_cyl;	/* low 8-bit of begin of cylinder, add 
					 * high 2-bit in beg_sect, total 10-bit
					 * cylinder, from 0-1023
					 */
	u_int8_t	sysid;		/* system identifier: 83 mean Linux */
	u_int8_t	end_head;	/* end of head, max 254 */
	u_int8_t	end_sect;	/* low 6-bit is end of sector, high 
					 * 2-bit is high of end of cylinder, 
					 * total 6-bit from 0-63.
					 */
	u_int8_t	end_cyl;	/* low 8-bit of end of cylinder, the 
					 * high 2-bit is high 2-bit of end of 
					 * sector, the total is 10-bit,
					 * from 0-1023.
					 */
	u_int32_t	start_sect;	/* start of real sector */
	u_int32_t	nr_sect;	/* number of sectors */
} _mbrpart_t;


/**
 *	Check the disk file @dev is a valid block device or not.
 *
 *	Return 0 if the disk is a valid block device, -1 if not.
 */
static int
_check_disk(const char *dev)
{
	struct stat st;

	if (!dev)
		return -1;

	if (stat(dev, &st))
		return -1;

	if (S_ISBLK(st.st_mode))
		return 0;

	return -1;
}


/**
 *	Check a disk is valid partitions, it find disk infomation in 
 *	"/proc/partitions".
 *
 *	Return 1 if disk is valid, 0 is invalid
 */
static int 
_find_diskproc(const char *disk, _diskproc_t *proc)
{
	FILE *fp;
	char line[256];
	char *ptr;

	fp = fopen(_HDISK_PROC, "r");
	if (!fp) {
		printf("can't open file %s\n", _HDISK_PROC);
		return 0;
	}

	while (fgets(line, 255, fp)) {

		memset(proc, 0, sizeof(_diskproc_t));
		sscanf(line, "%d %d %u %s", &proc->major, 
		       &proc->minor, &proc->nblocks, proc->name);

		if (proc->major < 1)
			continue;

		ptr = strrchr(disk, '/');
		if (ptr)
			ptr++;
		else
			ptr = (char *)disk;

		if (strcmp(proc->name, ptr) == 0) {
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	
	return -1;
}


/**
 *	Check disk @disk is a valid disk in system, it need /proc filesystem 
 *	mounted.
 *
 *	Return 1 if valid, 0 if invalid.
 */
extern int 
hddisk_valid(const char *disk)
{
	_diskproc_t proc;

	if (_check_disk(disk))
		return 0;

	if (_find_diskproc(disk, &proc)) {
		return 0;
	}

	return 1;
}

/**
 *	Get the sector number of disk @disk, it need /proc filesystem mounted.
 *
 *	Return the sector number of disk if success, -1 on error.
 */
extern long 
hddisk_size(const char *disk)
{
	_diskproc_t proc;

	if (_check_disk(disk))
		return -1;

	if (_find_diskproc(disk, &proc))
		return -1;

	return (proc.nblocks * 2);
}


/**
 *	Find disk mount entry in file "/proc/mounts", if found, the
 *	mount information is stored in @proc.
 *
 *	Return 0 if found, -1 means not found or error.
 */
static int 
_find_mountproc(const char *disk, _mountproc_t *proc)
{
	FILE *fp;
	char line[256];

	fp = fopen(_HDMOUNT_PROC, "r");
	if (!fp)
		return -1;

	while (fgets(line, 255, fp)) {

		memset(proc, 0, sizeof(_mountproc_t));
		sscanf(line, "%s %s %s %s %d %d", proc->dev, proc->mnt, 
		       proc->type, proc->opt, &proc->dump, &proc->pass);

		if (strcmp(proc->dev, disk) == 0) {
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	return -1;
}


/**
 *	mount a disk @disk to a mount pointer @mnt, it'll try use "ext2",
 *	"ext3", "reiserfs" type to mount.
 *
 *	Return 0 if mount success.
 */
extern int 
hddisk_mount(const char *disk, const char *mnt)
{
	if (_check_disk(disk) || !mnt)
		return -1;

	/* Try mount ext3 first, because ext3 fs can mounted as
	 * ext2 
	 */
	if (mount(disk, mnt, "ext3", MS_MGC_VAL, NULL) == 0) {
		printf("mount %s to %s ext3 success\n", disk , mnt);
		sync();
		return 0;
	}

	if (mount(disk, mnt, "ext2", MS_MGC_VAL, NULL) == 0) {
		printf("mount %s to %s ext2 success\n", disk , mnt);
		sync();
		return 0;
	}

	if (mount(disk, mnt, "reiserfs", MS_MGC_VAL, NULL) == 0) {
		printf("mount %s to %s reiserfs success\n", disk , mnt);
		sync();
		return 0;
	}

	return -1;
}

/**
 *	umount a disk @disk if it's mount to a pointer, it need "/proc" is mounted.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
hddisk_umount(const char *disk)
{
	_mountproc_t proc;

	if (_check_disk(disk))
		return -1;

	while (_find_mountproc(disk, &proc) == 0) {
		printf("%s is mounted at %s\n", proc.dev, proc.mnt);
		umount(proc.mnt);
	}

	return 0;
}


/**
 *	Check disk @disk is mounted or not. it need "/proc" is mounted
 *
 *	Return 1 if mounted , 0 is not mounted.
 */
extern int 
hddisk_is_mounted(const char *disk)
{
	_mountproc_t proc;

	if (_check_disk(disk))
		return -1;

	if (_find_mountproc(disk, &proc)) 
		return 0;

	return 1;
}


/**
 *	Partition a disk according partition table @partbl, it can only 
 *	fdisk a disk into no more than 4 main partitions, it didn't 
 *	support logical partition. If the @part is NULL, the fdisk 
 *	the whole disk one partitions.
 *
 *	Return 0 if fdisk success, -1 on error.
 */
extern int 
hddisk_set_partitions(const char *dev, hddisk_partbl_t *partbl)
{
	char tbl[64];
	_mbrpart_t *part;
	int i;
	int fd;
	int n;
	int signature = 0xAA55;

	if (_check_disk(dev) || !partbl)
		return -1;

	memset(tbl, 0, sizeof(tbl));
	part = (_mbrpart_t *)tbl;
	
	for (i = 0; i < partbl->size; i++) {
		if (partbl->tbl[i].active) 
			part[i].bootid = 0x80;
		
		part[i].start_sect = partbl->tbl[i].begin;
		part[i].nr_sect = partbl->tbl[i].size;
		part[i].sysid = partbl->tbl[i].sysid;
	}

	fd = open(dev, O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	lseek(fd, 446L, SEEK_SET);
	
	n = write(fd, part, 64);
	if (n != 64) {
		perror("write\n");
		return -1;
	}
	n = write(fd, &signature, 4);
	if (n != 4) {
		perror("write\n");
		return -1;
	}

	if (ioctl(fd, BLKRRPART, 0)) {
		perror("ioctl\n");
		return -1;
	}

	return 0;
}

/**
 *	Read the partition table from disk @disk and stored in @part.
 *
 *	Return 0 if read success, -1 on error.
 */
extern int
hddisk_get_partitions(const char *dev, hddisk_partbl_t *partbl)
{
	if (_check_disk(dev) || !partbl)
		return -1;

	return 0;
}


/**
 *	Clear all data in disk @disk. If the disk is whole disk, then the 
 *	partition table will cleared, all bytes in disk is zero. 
 *	
 *	Return 0 if success, -1 on error.
 */
extern int
hddisk_clear(const char *dev)
{
	int fd;
	char buf[_HDBLOCK_SIZE];
	int n;
	int m = 0;

	if (_check_disk(dev))
		return -1;

	fd = open(dev, O_RDWR);
	if (fd < 0)
		return -1;
	
	memset(buf, 0, sizeof(buf));
	while (1) {
		n = write(fd, buf, _HDBLOCK_SIZE);
		if (n <= 0)
			break;
		m++;
	}
	fsync(fd);

	/* reload partition table */
	if (ioctl(fd, BLKRRPART, 0)) {
		perror("ioctl\n");
		return -1;
	}

	close(fd);

	return 0;
}

/**
 *	Get the geometry of a hard disk @disk, and stored the geometry info 
 *	into @geo. because in linux header file, the cylinders is short int,
 *	so the disk size is not larger than 422G. use hddisk_size when
 *	in large disk(larger than 422G).
 *
 *	Return 0 if success, -1 on error.
 */
extern int
hddisk_get_geometry(const char *dev, hddisk_geometry_t *geo)
{
	struct hd_geometry hdgeo;
	int fd;

	if (!dev || !geo)
		return -1;
	
	fd = open(dev, O_RDONLY);
	if (fd < 0)
		return -1;
	
	if (ioctl(fd, HDIO_GETGEO, &hdgeo) < 0)
		return -1;
	
	geo->nsectors = hdgeo.sectors;
	geo->nheads = hdgeo.heads;
	geo->ncyls = hdgeo.cylinders;
	geo->start = hdgeo.start;

	return 0;
}


/**
 *	create a ext3 filesystem on disk @disk. it use @tool as format
 *	tool, example "/sbin/mkfs.ext2".
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
hddisk_mke3fs(const char *disk)
{
	char *args[9];
        int st;
        pid_t pid;
        static char *envp[] = {
                "TERM=vt100",
                "PATH=/bin:/sbin",
                NULL
        };

	args[0]= "/data/bin/mke2fs";
	args[1]= "-j";
	args[2]= "-F";
	args[3] = "-O";
	args[4] = "dir_index";
	args[5] = "-b";
	args[6] = "4096";
	args[7] = (char *)disk;
	args[8] = NULL;

        if ( (pid = fork()) == 0) {
                execve("/data/bin/mke2fs", args, envp);
                exit(-1);
        }

        wait (&st);
        if (st == 0) { 
		fflush(NULL);
		return 0;
	}
        else {
		return -1;
	}
}


