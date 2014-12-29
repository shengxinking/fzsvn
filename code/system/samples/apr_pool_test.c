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

#include <apr_pools.h>

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
	apr_pool_initialize();
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
_apr_pool_test(void)
{
	apr_pool_t *pool;
	int ret;
	char *ptr1;
	char *ptr2;

	ret = apr_pool_create(&pool, NULL);
	if (ret != APR_SUCCESS) {
		printf("apr_pool_create failed\n");
		return -1;
	}

	ptr1 = apr_pool_pcalloc(pool, 1024);
	ptr2 = apr_pool_pcalloc(pool, 4096);

	apr_pool_destroy(&pool);
	
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

	_apr_pool_test();

	_release();

	return 0;
}



