/*
 *	@file	disk.c
 *
 *	@brief	hard disk APIs
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-10-08
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DISK_PROC	"/proc/partitions"

typedef struct diskproc {
	int	major;
	int	minor;
	long	nblocks;
	char	name[64];
} diskproc_t ;

/**
 *	Check a disk is valid partitions
 *
 *	Return 1 if disk is valid, 0 is invalid
 */

static int 
_find_diskproc(const char *disk, diskproc_t *proc;)
{
	FILE *fp;
	char line[256];

	if (!disk || !proc)
		return 0;

	fp = fopen(DISK_PROC, "r");
	if (!fp) {
		printf("can't open file %s\n", DISK_PROC);
		return 0;
	}

	while (fgets(line, 255, fp)) {
		sscanf(line, "%d %d %ld %s", &proc->major, 
		       &proc->minor, &proc->nblocks, proc->name);

		printf("major %d, minor %d, blocks %ld, name %s\n",
		       &proc->major, &proc->minor, 
		       &proc->nblocks, proc->name);

		if (strcmp(proc->name, disk) == 0) {
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	
	return -1;
}


/**
 *	Get the disk @dev sector number.
 *
 *	Return the disk sector number if success, -1 on error.
 */
long disk_size(const char *disk)
{
	diskproc_t diskproc;

	if (!disk)
		return -1;

	if (_find_diskproc(disk, &diskproc)) {
		return -1;
	}

	return (diskproc->nblocks * 2);
}


/**
 *	Check disk @disk is a valid disk recognised by kernel
 *
 *	Return 1 if valid, 0 is invalid
 */
int 
disk_valid(const char *disk)
{
	diskproc_t diskproc;

	if (!disk)
		return 0;

	if (_find_diskproc(disk, &diskproc)) {
		return 0;
	}

	return 1;
}




