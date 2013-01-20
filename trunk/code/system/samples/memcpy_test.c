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
#include <sys/time.h>


static int	_g_count = 1000000;
static char	*_g_buffer = NULL;
static char	*_g_dstbuf = NULL;
static int	_g_buflen = 4096;


/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
	printf("memcpy_test <options>\n");
	printf("\t-n\tloop count\n");
	printf("\t-s\tbuffer size\n");
	printf("\t-h\tshow help message\n\n");
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
	char optstr[] = ":n:s:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'n':
			_g_count = atoi(optarg);
			if (_g_count < 0)
				return -1;
			break;

		case 's':
			_g_buflen = atoi(optarg);
			if (_g_buflen < 0)
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
	_g_buffer = malloc(_g_buflen);
	if (!_g_buffer)
		return -1;

	_g_dstbuf = malloc(_g_buflen);
	if (!_g_dstbuf)
		return -1;

	memset(_g_buffer, 'c', _g_buflen);

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
	struct timeval begin, end;
	int sec, usec;
	u_int64_t nbyte, nmbyte;
	int i;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	gettimeofday(&begin, NULL);
	for (i = 0; i < _g_count; i++) {
		memcpy(_g_dstbuf, _g_buffer, _g_buflen);
	}
	gettimeofday(&end, NULL);

	nbyte = (u_int64_t)_g_buflen * _g_count;
	nmbyte = nbyte/1024/1024;
	if (end.tv_usec < begin.tv_usec) {
		end.tv_usec += 1000000;
		end.tv_sec--;
	}
	sec = end.tv_sec - begin.tv_sec;
	usec = end.tv_usec - begin.tv_usec;

	printf("copy %lu bytes %lu MBytes %d sec %d usec\n",
		nbyte, nmbyte, sec, usec);

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






