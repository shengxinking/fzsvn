/*
 *	@file	hddisk_test.c
 *
 *	@brief	test all APIs in hddisk.c
 *	
 *	@date	2008-10-07
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "hddisk.h"

static int _g_showgeo = 0;
static int _g_partition = 0;
static int _g_size = 0;
static int _g_mount = 0;
static int _g_umount = 0;
static int _g_format = 0;
static int _g_clear = 0;
static char _g_dev[64];
static char _g_mnt[64];

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("hddisk_test <options>\n");
	printf("\t-g\tshow disk geometry\n");
	printf("\t-f\tpartition disk\n");
	printf("\t-s\tshow disk size\n");
	printf("\t-m\tmount disk if -p is used, or check disk is mount or not\n");
	printf("\t-u\tumount disk if it's mount\n");
	printf("\t-p\tmount pointer, if not set, -m only chekc disk is mounted or not\n");
	printf("\t-c\tclear disk data\n");
	printf("\t-F\tformat disk using ext3fs\n");
	printf("\t-d\tdisk name\n");
	printf("\t-h\tshow help infomation\n");
}

/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":gfsmucFd:p:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'g':
			_g_showgeo = 1;
			break;

		case 'f':
			_g_partition = 1;
			break;

		case 'c':
			_g_clear = 1;
			break;

		case 'F':
			_g_format = 1;
			break;

		case 's':
			_g_size = 1;
			break;

		case 'm':
			_g_mount = 1;
			break;

		case 'u':
			_g_umount = 1;
			break;

		case 'p':
			strncpy(_g_mnt, optarg, 63);
			break;

		case 'd':
			strncpy(_g_dev, optarg, 63);
			break;

		case 'h':
			return -1;

		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind)
		return -1;

	if (strlen(_g_dev) < 1)
		return -1;

	return 0;
}

/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	return 0;
}


/**
 *	Release global resource alloced by _initiate().	
 *
 * 	No Return.
 */
static void 
_release(void)
{
}

/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	hddisk_geometry_t geo;
	hddisk_partbl_t part;
	long size = 0;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	if (_g_size) {
		size = hddisk_size(_g_dev);
		if (size > 0) 
			printf("disk %s size is %ld\n", _g_dev, size);
		else
			printf("can't get disk %s size\n", _g_dev);

		return 0;
	}

	if (_g_showgeo) {
		if (hddisk_get_geometry(_g_dev, &geo))
			printf("can't get disk %s geometry\n", _g_dev);
		else{
			printf("disk %s geo is: sectors: %u, heads %u, cylinders %u\n",
			       _g_dev, geo.nsectors, geo.nheads, geo.ncyls);
		}

		return 0;
	}

	if (_g_mount) {

		if (strlen(_g_mnt) < 1) {
			if (hddisk_is_mounted(_g_dev))
				printf("%s is mounted\n", _g_dev);
			else
				printf("%s is not mounted\n", _g_dev);
		}
		else {
			if (hddisk_mount(_g_dev, _g_mnt))
				printf("mount %s to %s error\n", _g_dev, _g_mnt);
			else
				printf("mount %s to %s success\n", _g_dev, _g_mnt);
		}

		return 0;
	}

	if (_g_umount) {
		if (hddisk_umount(_g_dev)) {
			printf("umount %s error\n", _g_dev);
		}
		else {
			printf("umount %s success\n", _g_dev);
		}

		return 0;
	}

	if (_g_clear) {
		if (hddisk_clear(_g_dev)) {
			printf("can't clear disk %s\n", _g_dev);
		}
		else {
			printf("clear disk %s success\n", _g_dev);
		}

		return 0;
	}

	if (_g_partition) {
		
		if (hddisk_get_geometry(_g_dev, &geo)) {
			printf("can't get disk %s geometry\n", _g_dev);
			return -1;
		}

		memset(&part, 0, sizeof(&part));
		part.tbl[0].active = 1;
		size = geo.nsectors * geo.nheads * geo.ncyls;

		/* skipped first track */
		part.tbl[0].begin = geo.nsectors;
		part.tbl[0].size =  size - geo.nsectors;
		part.tbl[0].sysid = 0x83;
		part.size = 1;

		if (hddisk_set_partitions(_g_dev, &part)) {
			printf("disk %s set partitions error\n", _g_dev);
		}
		else {
			printf("disk %s set partitions success\n", _g_dev);
		}
	}
	
	if (_g_format) {
		if (hddisk_mke3fs(_g_dev)) {
			printf("Format %s failed\n", _g_dev);
		}
		else {
			printf("Format %s success\n", _g_dev);
		}
	}

	_release();

	return 0;
}

