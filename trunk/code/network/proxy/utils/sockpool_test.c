/*
 *	Test program for socket pool. It's a multi-thread program
 *	each thread get/put socket from socket pool concurrency.
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "sockpool.h"

#define _MAX_THREADS		32

static pthread_t _g_thread_ids[_MAX_THREADS];
static int _g_thread_num = 1;
static int _g_loop_times = 1;
static int _g_poolsize = 8;
static u_int32_t _g_ip = 0;
static u_int16_t _g_port = 8080;
static sockpool_t *_g_sockpool;

static void 
_usage(void)
{
	printf("sockpool_test [options]\n");
	printf("\t-n\tthread numbers, default is 1\n");
	printf("\t-t\tloop times, default is 1\n");
	printf("\t-s\tsocket pool size, default is 1\n");
	printf("\t-a\tserver address\n");
	printf("\t-p\tserver port\n");
	printf("\t-h\tshow help message\n");
}


static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":n:t:s:a:p:h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {

		case 'n':
			_g_thread_num = atoi(optarg);
			if (_g_thread_num < 1 || _g_thread_num > _MAX_THREADS)
				return -1;
			break;

		case 't':
			_g_loop_times = atoi(optarg);
			if (_g_loop_times < 1)
				return -1;
			break;
			
		case 's':
			_g_poolsize = atoi(optarg);
			if (_g_poolsize < 1)
				return -1;
			break;

		case 'a':
			_g_ip = inet_addr(optarg);
			break;

		case 'p':
			_g_port = atoi(optarg);
			break;

		case 'h':
			return -1;

		case ':':
			printf("option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind)
		return -1;

	if (!_g_ip || !_g_port)
		return -1;

	return 0;
}

static int
_initiate(void)
{
	if (_g_thread_num > 1)
		_g_sockpool = sockpool_alloc(_g_poolsize, 1);
	else
		_g_sockpool = sockpool_alloc(_g_poolsize, 0);

	if (!_g_sockpool)
		return -1;

	return 0;
}

static void
_release(void)
{
	if (_g_sockpool) {
		sockpool_free(_g_sockpool);
		_g_sockpool = NULL;
	}
}

static void *
_thread_run(void *arg)
{
	int i;
	int fd;
	int success = 0;
	int failed = 0;


	for (i = 0; i < _g_loop_times; i++) {
		fd = sockpool_get(_g_sockpool, _g_ip, _g_port);
		if (fd < 0) {
			failed ++;
			continue;
		}
		sleep(30);
		sockpool_print(_g_sockpool);
		if (sockpool_put(_g_sockpool, fd, 0)) {
			failed ++;
			continue;
		}

		success++;
	}

	printf("Try %d times, success %d times, failed %d times\n",
	       _g_loop_times, success, failed);

	return NULL;
}

int 
main(int argc, char **argv)
{
	int i;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		_release();
		return -1;
	}

	sockpool_print(_g_sockpool);

	if (_g_thread_num > 1) {
		for (i = 0; i < _g_thread_num; i++) {
			if (pthread_create(&_g_thread_ids[i], NULL, _thread_run, NULL)) {
				printf("create thread failed: %s\n", strerror(errno));
			}
			printf("create thread %lu\n", _g_thread_ids[i]);
		}
	}
	else {
		_thread_run(NULL);
	}

	if (_g_thread_num > 1) {
		for (i = 0; i < _g_thread_num; i++) {
			if (pthread_join(_g_thread_ids[i], NULL)) {
				printf("pthread_join(%lu) error: %s\n", 
				       _g_thread_ids[i], strerror(errno));
			}
			printf("stop thread %lu\n", _g_thread_ids[i]);
		}
	}

	sockpool_print(_g_sockpool);

	_release();

	return 0;
}
