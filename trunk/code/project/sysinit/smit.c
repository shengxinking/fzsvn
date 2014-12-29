/**
 *	@file	smit.c
 *
 *	@brief	The smit program, it'll do following things:
 *		startup		the system startup.
 *		mount		mount all filesystem.
 *		umount		unmount not needed filesystem in reload.
 *		raid		create raid.
 *		format		format log/flash disk.
 *		upgrade		upgrade the image.
 *		update		update the config file.
 *
 *	@author	Forrest.zhang	
 *
 *	@date	2010-08-26
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "system.h"
#include "network.h"
#include "filesys.h"
#include "hardware.h"


static void 
_usage(void)
{
	printf("smit\t the command for init process\n");
	printf("\tstartup\tthe system startup\n");
	printf("\tmount\tmount the filesystem\n");
	printf("\tumount\tumount all filesystem\n");
	printf("\traid\tcreate raid\n");
	printf("\tformat\tformat the disk\n");
	printf("\tupgrade\tupgrade image\n");
	printf("\tupdate\tupdate the config\n");
}


int 
startup_main(int argc, char **argv)
{
	sys_insmod();

	fs_mount_sys();
	
	fs_mount_disk();

	nw_chname();

	nw_set_lo();

	hw_init_xmlcp();

	hw_init_bypass();

	sys_init_cmdb();

	sys_update_fname();

	sys_extract_files();

	return 0;
}

int 
mount_main(int argc, char **argv)
{
	fs_mount_sys();

	fs_mount_disk();

	return 0;
}

int 
umount_main(int argc, char **argv)
{
	fs_umount_disk();

	return 0;
}

int 
raid_main(int argc, char **argv)
{

	return 0;
}

int 
format_main(int argc, char **argv)
{

	return 0;
}

int 
upgrade_main(int argc, char **argv)
{

	return 0;
}


int 
update_main(int argc, char **argv)
{
	return 0;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		_usage();
		return -1;
	}

	if      (strcmp(argv[1], "startup") == 0) 
		return startup_main(argc - 1, &argv[1]);
	else if (strcmp(argv[1], "mount") == 0)
		return mount_main(argc - 1, &argv[1]);
	else if (strcmp(argv[1], "umount") == 0)
		return umount_main(argc - 1, &argv[1]);
	else if (strcmp(argv[1], "raid") == 0)
		return raid_main(argc - 1, &argv[1]);
	else if (strcmp(argv[1], "format") == 0)
		return format_main(argc - 1, &argv[1]);
	else if (strcmp(argv[1], "upgrade") == 0)
		return upgrade_main(argc - 1, &argv[1]);
	else if (strcmp(argv[1], "update") == 0)
		return update_main(argc - 1, &argv[1]);
	else {
		_usage();
		return -1;
	}
}




