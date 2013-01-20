/**
 *	@file	dashboard_test.c
 *
 *	@brief	The dashboard API test program.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2010-10-20
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "dashboard.h"

#define _CREATE_MODE	0
#define _SHOW_MODE	1
#define _UPDATE_MODE	2
#define _DELETE_MODE	3

#define _SHM_KEY	0x12345432


typedef struct proxy_info {
	pid_t		pid;
	u_int64_t	nrecvs, nsends;
	u_int32_t	timestamp;
} proxy_info_t;

static int 		_g_mode = 0;
static dashboard_t	*_g_dbd = NULL;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("dashboard_test <options>\n\n");
	printf("\t-c\tCreate dashboard\n");
	printf("\t-s\tShow dashboard content\n");
	printf("\t-u\tUpdate dashboard content\n");
	printf("\t-d\tDelete dashboard\n");
	printf("\t-h\tShow help message\n");
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
	char optstr[] = ":csudh";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'c':
			_g_mode = _CREATE_MODE;
			break;
		case 's':
			_g_mode = _SHOW_MODE;
			break;

		case 'u':
			_g_mode = _UPDATE_MODE;
			break;

		case 'd':
			_g_mode = _DELETE_MODE;
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


static int 
_do_create(void)
{
	proxy_info_t *info;

	_g_dbd = dbd_alloc(_SHM_KEY, sizeof(proxy_info_t));
	if (!_g_dbd) {
		printf("create dashboard failed\n");
		return -1;
	}
	
	dbd_lock(_g_dbd);

	info = (proxy_info_t *)_g_dbd->data;

	info->pid = getpid();
	info->timestamp = time(NULL);

	dbd_unlock(_g_dbd);

	return 0;
}

static int 
_do_show(void)
{
	proxy_info_t *info;
	int i;

	_g_dbd = dbd_attach(_SHM_KEY);
	if (!_g_dbd)
		return -1;

	for (i = 0; i < 1000; i++) {
		dbd_lock(_g_dbd);
		info = (proxy_info_t *) _g_dbd->data;
		printf("--------Begin--------\n");
		printf("pid:\t%d\n", info->pid);
		printf("nrecvs:\t%llu\n", info->nrecvs);
		printf("nsends:\t%llu\n", info->nsends);
		printf("time:\t%d\n", info->timestamp);
		printf("---------End---------\n\n");
		dbd_unlock(_g_dbd);
		sleep(1);
	}

	return 0;
}

static int 
_do_update(void)
{
	proxy_info_t *info;
	int i;

	_g_dbd = dbd_attach(_SHM_KEY);
	if (!_g_dbd)
		return -1;

	for (i = 0; i < 1000; i++) {
		dbd_lock(_g_dbd);
		info = (proxy_info_t *) _g_dbd->data;
		info->nrecvs += 100;
		info->nsends += 100;
		info->timestamp = time(NULL);
		dbd_unlock(_g_dbd);
		sleep(1);
	}

	return 0;
}

static int 
_do_delete(void)
{
	_g_dbd = dbd_attach(_SHM_KEY);

	if (!_g_dbd)
		return -1;

	dbd_free(_g_dbd);

	return 0;
}


/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}
	
	if (_initiate()) {
		return -1;
	}

	switch (_g_mode) {

	case _CREATE_MODE:
		_do_create();
		break;

	case _SHOW_MODE:
		_do_show();
		break;

	case _UPDATE_MODE:
		_do_update();
		break;

	case _DELETE_MODE:
		_do_delete();
		break;

	default:
		break;
	}

	_release();

	return 0;
}



