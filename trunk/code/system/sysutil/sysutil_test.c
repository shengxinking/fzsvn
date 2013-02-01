/**
 *	@file	sysutil_test.c
 *
 *	@brief	test program for all sysutil APIs.
 *
 *	@author	Forrest.zhang
 *	
 *	@date	2012-07-23
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include "sysutil.h"


enum {
	CPU_TEST = 0,
	MEM_TEST,
	PROC_TEST,
	PIDFILE_TEST,
	PROCFILE_TEST,
};

static int _g_type = CPU_TEST;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("sysutil_test <options>\n");
	printf("\t-c\tcpu API test\n");
	printf("\t-m\tmemory API test\n");
	printf("\t-p\tprocess API test\n");
	printf("\t-f\tpidfile API test\n");
	printf("\t-o\tprocfile API test\n");
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
	char optstr[] = ":cmpfoh";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 'c':
			_g_type = CPU_TEST;
			break;

		case 'm':
			_g_type = MEM_TEST;
			break;

		case 'p':
			_g_type = PROC_TEST;
			break;
			
		case 'f':
			_g_type = PIDFILE_TEST;
			break;
			
		case 'o':
			_g_type = PROCFILE_TEST;
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
_cpu_test(void)
{
	int cpus[128] = {0};
	int ncpu;
	int i;

	ncpu = cpu_number();

	printf("cpu usage is: %d%%\n", cpu_total_usage());
	cpu_all_usage(cpus, ncpu);
	for (i = 0; i < ncpu; i++) {
		printf("  cpu%d: %d%%\n", i + 1, cpus[i]);
	}
	printf("cpu freq is %dMHz\n", cpu_freq());
	printf("cpu number is %d\n", ncpu);
}

static void 
_mem_test(void)
{
	printf("mem total %dMB\n", mem_total());
	printf("mem free %dMB\n", mem_freed());
	printf("mem used %dMB\n", mem_used());
	printf("mem usage %d%%\n", mem_usage());

	return;
}


static void 
_proc_test(char **argv)
{
	char *ptr, *ptr1, *ptr2;

	printf("pid is %d\n", getpid());

	if (proc_rename("renamed_process", argv)) {
		printf("change process name failed");
	}
	else {
		printf("renamed to renamed_process\n");
	}

	printf("rss is %lu KB\n", proc_rss());

	ptr = malloc(40960);
	memset(ptr, 0, 40960);
	
	sleep(10);
	printf("rss is %lu KB\n", proc_rss());

	ptr1 = malloc(4096);
	memset(ptr1, 0, 4096);
	ptr2 = malloc(4096);
	memset(ptr2, 0, 4096);

	sleep(10);
	printf("rss is %lu KB\n", proc_rss());

	free(ptr);
	free(ptr1);
	free(ptr2);
	sleep(10);
	printf("rss is %lu KB\n", proc_rss());
}

static void 
_pidfile_test(void)
{

}

static void 
_procfile_test(void)
{
	int rss = 0;

	rss = procfile_get_rss(1);

	printf("rss is %d\n", rss);
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
		
	case CPU_TEST:
		_cpu_test();
		break;

	case MEM_TEST:
		_mem_test();
		break;

	case PROC_TEST:
		_proc_test(argv);
		break;

	case PIDFILE_TEST:
		_pidfile_test();
		break;

	case PROCFILE_TEST:
		_procfile_test();

	default:
		break;
	}

	_release();

	return 0;
}



