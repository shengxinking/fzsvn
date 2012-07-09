/**
 *	@file	
 *
 *	@brief
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
#include <pthread.h>


static pthread_mutex_t	_g_lock;


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

static void 
_cleanup(void)
{
	printf("call cleanup\n");

	if (pthread_mutex_unlock(&_g_lock)) {
		if (errno == EPERM) {
			printf("mutex is not locked\n");
		}
		else {
			printf("unlock mutex when atexit\n");
		}
	}
}


/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);

	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

	pthread_mutex_init(&_g_lock, &attr);

	if (atexit(_cleanup)) {
		printf("Add exit cleanup function failed\n");
		return -1;
	}
	
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
_segfault(void)
{
	char *ptr = 0x0;

	*ptr = 'c';

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

	if (pthread_mutex_lock(&_g_lock)) {
		printf("mutex lock failed: %s\n", strerror(errno));
	}

	if (pthread_mutex_lock(&_g_lock)) {
		printf("mutex lock(2) failed(%d): %s\n", errno, strerror(errno));
	}

	if (pthread_mutex_unlock(&_g_lock)) {
		printf("mutex unlock failed: %s\n", strerror(errno));
	}


	if (pthread_mutex_unlock(&_g_lock)) {
		printf("mutex unlock(2) failed(%d): %s\n", errno, strerror(errno));
	}

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






