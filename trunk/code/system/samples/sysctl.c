/**
 *	@file	sysctl_test.c
 *
 *	@brief	sysctl function.
 *	
 *	@date	2008-12-19
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/sysctl.h>


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
	struct __sysctl_args args;
	int name[] = { CTL_KERN, KERN_OSREV, NULL};
	int oldval = 0, newval = 0;
	size_t oldlen = 0;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	memset(&args, 0, sizeof(args));
	args.name = name;
	args.nlen = sizeof(name) / sizeof(int);
	args.oldval = &oldval;
	oldlen = sizeof(oldval);
	args.oldlenp = &(oldlen);

	if (syscall(SYS__sysctl, &args)) {
		printf("sysctl error: %s\n", strerror(errno));
		return -1;
	}

	printf("sysctl return value is %d\n", oldval);

	_release();

	return 0;
}



