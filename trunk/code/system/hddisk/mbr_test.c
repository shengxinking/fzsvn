/*
 *	@file	mbr_test.c
 *
 *	@brief
 *	
 *	@date
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <getopt.h>
#include <string.h>

#include "mbr.h"

#define	_NAMELEN	24

static char _g_devname[_NAMELEN];

static void _usage(void)
{
	printf("mbr_test <options>\n");
	printf("\t-d\tdevice name\n");
	printf("\t-h\tshow help message\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":d:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'd':
			strncpy(_g_devname, optarg, _NAMELEN - 1);
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

	if (strlen(_g_devname) < 1)
		return -1;

	return 0;
}

static int _initiate(void)
{
	return 0;
}


static void _release(void)
{
}


int main(int argc, char **argv)
{
	mbr_t mbr;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	if (mbr_get(_g_devname, &mbr)) {
		printf("get %s MBR error\n", _g_devname);
	}
	else 
		mbr_print(&mbr);

	_release();

	return 0;
}





