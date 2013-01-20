/*
 *	@file	memperf.c
 *
 *	@brief	A simple test program to test memmove or memcpy performance.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-07-09
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>

static int _g_bsize = 1;
static int _g_bnum = 1;
static char *_g_inbuf = NULL;
static char *_g_outbuf = NULL;

/**
 *	Show program usage.
 *
 *	No return.
 */
static void 
_usage(void)
{
	printf("memperf <options>\n");
	printf("\t-s\tblock size\n");
	printf("\t-n\tblock number\n");
	printf("\t-h\tshow help message\n");
} 


/**
 *	Parse command line argument.
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char optstr[] = ":s:n:h";
	char opt;

	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {

		case 's':
			_g_bsize = atoi(optarg);
			if (_g_bsize < 1)
				return -1;
			break;

		case 'n':
			_g_bnum = atoi(optarg);
			if (_g_bnum < 1)
				return -1;
			break;

		case 'h':
			return -1;

		case ':':
			printf("option %c missing argument\n", opt);
			return -1;

		case '?':
			printf("unknowed option %c\n", opt);
			return -1;
		}
	}

	if (optind != argc)
		return -1;

	return 0;
}


/**
 *	Initiate global resource.
 *
 *	Return 0 if success, -1 on error.
 */
static int
_initiate(void)
{
	_g_inbuf = malloc(_g_bsize);
	if (!_g_inbuf) {
		printf("malloc inbuf error: %s\n",
		       strerror(errno));
		return -1;
	}
	memset(_g_inbuf, 'A', _g_bsize);

	_g_outbuf = malloc(_g_bsize);
	if (!_g_outbuf) {
		printf("malloc outbuf error: %s\n",
		       strerror(errno));
		return -1;
	}

	return 0;	
}


/**
 *	Free global resource.
 *
 *	No return.
 */
static void
_release(void)
{
	if (_g_inbuf)
		free(_g_inbuf);
	_g_inbuf = NULL;

	if (_g_outbuf)
		free(_g_outbuf);
	_g_outbuf = NULL;
}


/**
 *	the main process function
 *
 *	Return 0 if success, -1 on error.
 */
static int 
_do_loop(void)
{
	struct timeval tv1, tv2;
	long sec, usec;
	float mbps;
	long long nbytes;
	int i;

	printf("count is %d, block is %d\n", _g_bsize, _g_bnum);

	i = _g_bnum;

	gettimeofday(&tv1, NULL);
	while (i--) {
		memmove(_g_outbuf, _g_inbuf, _g_bsize);
	}
	gettimeofday(&tv2, NULL);
	
	if (tv2.tv_usec < tv1.tv_usec) {
		usec = 1000000 + tv2.tv_usec - tv1.tv_usec;
		tv2.tv_sec--;
	}
	else
		usec = tv2.tv_usec - tv1.tv_usec;

	sec = tv2.tv_sec - tv1.tv_sec;
	nbytes = _g_bsize * _g_bnum;
	mbps = ( (nbytes * 8) / (1024 * 1024.0) ) / (sec + usec/1000000.0);

	printf("memmove %lld bytes, spend %ld sec, %ld msec, %f Mbps\n",
	       nbytes, sec, usec, mbps);

	/* test memmove performance */

	i = _g_bnum;

	gettimeofday(&tv1, NULL);
	while (i--) {
		memcpy(_g_outbuf, _g_inbuf, _g_bsize);
	}
	gettimeofday(&tv2, NULL);
	
	if (tv2.tv_usec < tv1.tv_usec) {
		usec = 1000000 + tv2.tv_usec - tv1.tv_usec;
		tv2.tv_sec--;
	}
	else
		usec = tv2.tv_usec - tv1.tv_usec;

	sec = tv2.tv_sec - tv1.tv_sec;
	nbytes = _g_bsize * _g_bnum;
	mbps = ( (nbytes * 8) / (1024 * 1024.0) ) / (sec + usec/1000000.0);

	printf("memcpy %lld bytes, spend %ld sec, %ld msec, %f Mbps\n",
	       nbytes, sec, usec, mbps);

	return 0;
}


/**
 *	The main entry of program
 *
 *	Return 0 if success, -1 on error.
 */
int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		_release();
		return -1;
	}

	_do_loop();

	_release();

	return 0;
}
