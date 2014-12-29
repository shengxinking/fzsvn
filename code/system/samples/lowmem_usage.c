/**
 *	@file	lowmem_usage.c
 *
 *	@brief	Check malloc used Low memory or High memory.
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

#define MB_SIZE		(1024 * 1024)

static long long _g_lowmem = 10; 


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("lowmem_usage <options>\n");
	printf("\t-s\tthe free low memory size(MB)\n");
	printf("\t-h\tshow the help message\n");
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
	char optstr[] = ":s:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 's':
			_g_lowmem = atoi(optarg);
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

	if (_g_lowmem < 1)
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
_do_loop(void)
{
	char *ptr;
	int nmb = _g_lowmem;

	while (nmb > 0) {
		ptr = malloc(MB_SIZE);
		if (!ptr) {
			printf("malloc failed\n");
			break;
		}

//		sleep(1);
		nmb--;
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

	_do_loop();

	sleep(10);

	_release();

	return 0;
}



