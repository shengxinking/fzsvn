/**
 *	@file	dbg_file_test
 *
 *	@brief	test program of dbg_file APIs.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2010-07-19
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "dbg_file.h"

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
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
	char optstr[] = ":h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
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
	dbg_file_init("dbg_file_test");

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
	dbg_file_release();
}

static int _dbg_file_test(void)
{
	int i = 0;
	while (1) {
		usleep(10);
		i++;
		
		if ((i % 100) == 0) {
			dbg_abnormal("there have a abnormal in %s:%d\n", 
				     __FILE__, __LINE__);
		}

		if ((i % 377) == 0) {
			dbg_deadlock("there have a deadlock in %s:%d\n", 
				     __FILE__, __LINE__);
		}

		if (i > 1000)
			dbg_infloop("there have a infloop in %s:%d\n", 
				    __FILE__, __LINE__);
		if (i == 1005)
			break;
	}

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

	_dbg_file_test();
	
	_release();

	return 0;
}



