/**
 *	@file	
 *
 *	@brief
 *
 *	@author	Forrest.zhang
 *	
 *	@date
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "dbg_print.h"
#include "dbg_log.h"
#include "dbg_segfault.h"

enum {
	_TEST_PRINT,
	_TEST_LOG,
	_TEST_SEGV,
};

static int		_g_type = _TEST_PRINT;


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("debug_test <options>\n");
	printf("\t-p\ttest debug print\n");
	printf("\t-l\ttest debug log\n");
	printf("\t-s\ttest debug segfault\n");
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
	char optstr[] = ":lps:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'p':
			_g_type = _TEST_PRINT;
			break;

		case 'l':
			_g_type = _TEST_LOG;
			break;

		case 's':
			_g_type = _TEST_SEGV;
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

static void 
_sig_segv(int signo)
{
	DBG_LOG_CRASH("debug_test received SEGV signal\n");
}


/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	dbg_segfault("debug_test", _sig_segv);

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
_test_print(void)
{
	dbg_print("Liujie is a SB\n");
	return 0;
}

static int 
_test_log(void)
{
	DBG_LOG_KERNEL("kernel is startup\n");
	return 0;
}

static int 
_test_segv(void)
{
	char *ptr = NULL;

	*ptr = 'a';
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

	switch (_g_type) {

	case _TEST_PRINT:
		_test_print();
		break;

	case _TEST_LOG:
		_test_log();
		break;

	case _TEST_SEGV:
		_test_segv();
		break;
	}
	
	_release();

	return 0;
}



