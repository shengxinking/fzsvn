/**
 *	@file	malloc_test.c
 *
 *	@brief	the malloc test program.
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
#include <malloc.h>
#include <sys/types.h>


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
	struct mallinfo mi;
	char *ptr1, *ptr2;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}
	
	printf("--------------- start --------------\n");
	mi = mallinfo();
	printf("arena:    %d\n", mi.arena);
	printf("ordblks:  %d\n", mi.ordblks);
	printf("smblks:   %d\n", mi.smblks);
	printf("hblks:    %d\n", mi.hblks);
	printf("hblkhd:   %d\n", mi.hblkhd);
	printf("usmblks:  %d\n", mi.usmblks);
	printf("fsmblks:  %d\n", mi.fsmblks);
	printf("uordblks: %d\n", mi.uordblks);
	printf("fordblks: %d\n", mi.fordblks);
	printf("keepcost: %d\n", mi.keepcost);
	malloc_stats();
	malloc_info(0, stdout);
	
	ptr1 = malloc(127 * 1024);
	ptr2 = malloc(81920);	
	memset(ptr1, 0, 127 * 1024);
	memset(ptr2, 0, 81920);
	printf("--------------- after malloc --------------\n");
	mi = mallinfo();
	printf("arena:    %d\n", mi.arena);
	printf("ordblks:  %d\n", mi.ordblks);
	printf("smblks:   %d\n", mi.smblks);
	printf("hblks:    %d\n", mi.hblks);
	printf("hblkhd:   %d\n", mi.hblkhd);
	printf("usmblks:  %d\n", mi.usmblks);
	printf("fsmblks:  %d\n", mi.fsmblks);
	printf("uordblks: %d\n", mi.uordblks);
	printf("fordblks: %d\n", mi.fordblks);
	printf("keepcost: %d\n", mi.keepcost);
	malloc_stats();
	malloc_info(0, stdout);

	free(ptr1);
	free(ptr2);
	printf("--------------- after free --------------\n");
	mi = mallinfo();
	printf("arena:    %d\n", mi.arena);
	printf("ordblks:  %d\n", mi.ordblks);
	printf("smblks:   %d\n", mi.smblks);
	printf("hblks:    %d\n", mi.hblks);
	printf("hblkhd:   %d\n", mi.hblkhd);
	printf("usmblks:  %d\n", mi.usmblks);
	printf("fsmblks:  %d\n", mi.fsmblks);
	printf("uordblks: %d\n", mi.uordblks);
	printf("fordblks: %d\n", mi.fordblks);
	printf("keepcost: %d\n", mi.keepcost);
	malloc_stats();
	malloc_info(0, stdout);

	_release();

	return 0;
}



/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */






