/**
 *	@file	filesys.h
 *
 *	@brief	The file system function for init.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2010-08-31
 */

#ifndef FZ_FILESYS_H
#define FZ_FILESYS_H

#define MAX_DEV_NAME	64

typedef struct mount_entry {
	char	ide[MAX_DEV_NAME];
	char	dev[MAX_DEV_NAME];
	char*	mpoint;
	char*	mtype;
	char*	mopt;
} mount_entry_t;

extern int 
fs_mount_sys(void);


extern int 
fs_mount_disk(void);


extern int 
fs_umount_disk(void);


extern int 
fs_format(const char *dev);


#endif /* end of FZ_  */

