/**
 *	@file	filesys.c
 *
 *	@brief	The APIs for filesystem.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include "filesys.h"

static mount_entry_t _sys_mnts[] = {
	{{0}, {"/dev/root"}, "/", "ext2", NULL},
	{{0}, {"none"}, "/proc", "proc", NULL},
	{{0}, {"none"}, "/sys", "sys", NULL},
	{{0}, {"none"}, "/tmp", "tmpfs", "size=15%"},
	{{0}, {"none"}, "/dev/pts", "devpts", NULL},
	{{0}, {"none"}, "/dev/shm", "tmpfs", NULL},
};

int 
fs_mount_sys(void)
{
	int i;
	int n;
	int ret;
	
	n = sizeof(_sys_mnts)/sizeof(mount_entry_t);

	for (i = 0; i < n; i++) {
		if (strcmp(_sys_mnts[i].mpoint, "/") == 0) {
			ret = mount(_sys_mnts[i].dev, _sys_mnts[i].mpoint,
				    NULL, MS_MGC_VAL | MS_REMOUNT, 
				    _sys_mnts[i].mopt);
		}
		else {
			ret = mount(_sys_mnts[i].dev, _sys_mnts[i].mpoint,
				    NULL, MS_MGC_VAL, _sys_mnts[i].mopt);
		}

		if (ret) {
			printf("mount %s to %s failed: %s\n", 
			       _sys_mnts[i].dev, _sys_mnts[i].mpoint,
			       strerror(errno));
		}
	}

	return 0;
}

static mount_entry_t _disk_mnts[] = {
	{{0}, {0}, "/data", "ext3", NULL},
	{{0}, {0}, "/home", "ext3", NULL},
	{{0}, {0}, "/var/log", "ext3", NULL},
};

int 
fs_mount_disk(void)
{
	int ret;
	int n;
	int i;

	n = sizeof(_disk_mnts) / sizeof(mount_entry_t);
	for (i = 0; i < n; i++) {
		ret = mount(_disk_mnts[i].dev, _disk_mnts[i].mpoint, NULL, 
			    MS_MGC_VAL, _disk_mnts[i].mopt);
		if (ret) {
			printf("mount %s to %s failed: %s\n", 
			       _disk_mnts[i].dev, _disk_mnts[i].mpoint, 
			       strerror(errno));
		}
	}

	return ret;
}

int 
fs_umount_disk(void)
{
	int ret;
	int n;
	int i;

	n = sizeof(_disk_mnts) / sizeof(mount_entry_t);
	for (i = 0; i < n; i++) {
		ret = umount(_disk_mnts[i].mpoint);
		if (ret) {
			printf("umount %s failed: %s\n", 
			       _disk_mnts[i].mpoint, strerror(errno));
		}
	}

	return ret;
}

int 
fs_format(const char *dev)
{
//	struct stat st;

	return 0;
}





