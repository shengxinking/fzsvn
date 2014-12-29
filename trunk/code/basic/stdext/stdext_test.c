/**
 *	@file	stdext_test.c
 *
 *	@brief	The test program for stdext APIs.
 *
 *	@author	Forrest.zhang
 *	
 *	@date
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "str_util.h"
#include "dir_util.h"

enum {
	TEST_STRING,
	TEST_DIR,
};

static int _g_type = TEST_STRING;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("stdext_test <options>\n");
	printf("\t-s\tstring test\n");
	printf("\t-d\tdir test\n");
	printf("\t-h\tshow help message\n");
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
	char optstr[] = ":sdh";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 's':
			_g_type = TEST_STRING;
			break;

		case 'd':
			_g_type = TEST_DIR;
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

static void 
_test_string(void)
{

}

static void 
_test_dir(void)
{
	if (dir_create("//tmp/test///test1//test2///test3/", 0755))
		printf("create dir failed\n");
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

	switch (_g_type) {

	case TEST_STRING:
		_test_string();
		break;

	case TEST_DIR:
		_test_dir();

	default:
		break;

	}

	_release();

	return 0;
}



