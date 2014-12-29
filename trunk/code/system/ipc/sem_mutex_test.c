/**
 *	@file	sem_mutex_test.c
 *
 *	@brief	sem_mutex API test program.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2012-07-30
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "sem_mutex.h"


static int	_g_key = 0x1234321;
static int 	_g_op = 0;
static int	_g_perf = 0;
static int	_g_verbose = 0;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("sem_mutex_test <options>\n");
	printf("\t-c\tcreate the mutex\n");
	printf("\t-d\tdestroy the mutex\n");
	printf("\t-l\tlock the mutex\n");
	printf("\t-u\tunlock the mutex\n");
	printf("\t-v\tprint the mutex information after each operation\n");
	printf("\t-k\tthe key id\n");
	printf("\t-p\tthe performance test\n");
	printf("\t-h\tshow help message\n");
}

enum {
	SEM_OP_CREATE	= 1 << 0,
	SEM_OP_DESTROY	= 1 << 1,
	SEM_OP_LOCK	= 1 << 2,
	SEM_OP_UNLOCK	= 1 << 3,
};


/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":cdluvk:ph";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'c':
			_g_op |= SEM_OP_CREATE;
			break;

		case 'd':
			_g_op |= SEM_OP_DESTROY;
			break;
			
		case 'l':
			_g_op |= SEM_OP_LOCK;
			break;

		case 'u':
			_g_op |= SEM_OP_UNLOCK;
			break;

		case 'v':
			_g_verbose = 1;
			break;

		case 'k':
			_g_key = atoi(optarg);
			break;

		case 'p':
			_g_perf = 1;
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
_sem_mutex_perf(void)
{

	return 0;
}

static int 
_crash(void)
{
	char *ptr = (char *)0x11111111;

	*ptr = 1;

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

	printf("current pid is %d\n", getpid());

	if (_g_perf) {
		_sem_mutex_perf();
		return 0;
	}

	if (_g_op & SEM_OP_CREATE) {
		if (sem_mutex_create(_g_key))
			return -1;
		if (_g_verbose)
			sem_mutex_print(_g_key);
	}

	if (_g_op & SEM_OP_LOCK) {
		if (sem_mutex_lock(_g_key))
			return -1;

		if (_g_verbose)
			sem_mutex_print(_g_key);

	}
	
	if (_g_op & SEM_OP_UNLOCK) {
		if (sem_mutex_unlock(_g_key))
			return -1;

		if (_g_verbose)
			sem_mutex_print(_g_key);

//		_crash();
	}

	if (_g_op & SEM_OP_DESTROY) {
		if (sem_mutex_destroy(_g_key))
			return -1;
	}

	if (_g_verbose)
		if (sem_mutex_print(_g_key))
			return -1;

	_release();

	return 0;
}



