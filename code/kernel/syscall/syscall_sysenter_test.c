/**
 *	@file	syscall_sysenter_test.c
 *
 *	@brief	a test program to test sysenter performance
 *	
 *	@date	2008-12-15
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/time.h>

static pid_t _g_pid;
static int _g_count = 1;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("Test sysenter intrust performance\n");
	printf("Usage: syscall_sysenter_test <options>\n");
	printf("\t-c\tsysenter call times\n");
	printf("\t-h\tshow help information\n");
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
	char optstr[] = ":c:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'c':
			_g_count = atoi(optarg);
			if (_g_count < 1) {
				printf("-c need a positive value");
				return -1;
			}
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


/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	int i;
	struct timeval tv1, tv2;
	unsigned long sec, usec;


	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	gettimeofday(&tv1, NULL);

	for (i = 0; i < _g_count; i++) {

		asm(
			"movl $20, %eax   \n"
			"call *%gs:0x10   \n"
			"movl %eax, _g_pid   \n"
			);

	}

	gettimeofday(&tv2, NULL);

	if (tv2.tv_usec >= tv1.tv_usec) {
		usec = tv2.tv_usec - tv1.tv_usec;
	}
	else {
		usec = 1000000 + tv2.tv_usec - tv1.tv_usec;
		tv2.tv_sec--;
	}
	
	sec = tv2.tv_sec - tv1.tv_sec;

	printf("sysenter syscall %d times spend %lu sec %lu usec\n",
	       _g_count, sec, usec);
	
	

	_release();

	return 0;
}



