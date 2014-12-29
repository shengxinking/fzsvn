/**
 *	@file	syscall_test.c	
 *
 *	@brief	test syscall performance
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
#include <sys/time.h>

#include <sys/syscall.h>

static int _g_count = 10000000;


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("syscall_test <options>\n");
	printf("\t-c <N>\tloop count\n");
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
	char optstr[] = ":c:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'c':
			_g_count = atoi(optarg);
			if (_g_count < 0)
				return -1;
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
	struct timeval begin, end;
	int cpu, node;
	int success = 0, failed = 0;
	u_int32_t nsec, nusec;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	gettimeofday(&begin, NULL);
	for (i = 0; i < _g_count; i++) {
		if (syscall(SYS_getcpu, &cpu, &node, NULL)) {
			failed ++;
		}
		else {
			success ++;
		}
	}
	gettimeofday(&end, NULL);

	_release();

	printf("try getcpu %d times, success %d times, failed %d times\n",
		_g_count, success, failed);
	if (end.tv_usec < begin.tv_usec) {
		nusec = end.tv_usec + 1000000 - begin.tv_usec;
		end.tv_sec--;
	}
	else
		nusec = end.tv_usec - begin.tv_usec;

	nsec = end.tv_sec - begin.tv_sec;
	printf("spend %d second, %d microsecond\n", nsec, nusec);

	return 0;
}



