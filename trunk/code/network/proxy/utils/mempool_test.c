/**
 *	@file	mempool_test.c
 *	@brief	It's a test program to test memory pool
 *	
 */ 

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

#include "mempool.h"

#define MAX_THREADS	32

static int _g_thread_num = 0;
static int _g_loop_times = 1;
static int _g_obj_size = 1;
static int _g_obj_num = 1;
static pthread_t _g_thread_ids[MAX_THREADS];
static mempool_t *_g_mempool = NULL;

static void 
_usage(void)
{
	printf("mempool_test <options>\n");
	printf("\t-n\tthread number, default is 1\n");
	printf("\t-s\tnumber of memory unit\n");
	printf("\t-o\tobject size\n");
	printf("\t-l\tloop times of get/put\n");
	printf("\t-h\tshow help message\n");
}


static int 
_parse_cmd(int argc, char **argv)
{
	char optstr[] = ":n:s:o:l:h";
	char c;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'n':
			_g_thread_num = atoi(optarg);
			if ( _g_thread_num < 1
			     || _g_thread_num > MAX_THREADS)
				return -1;
			break;

		case 's':
			_g_obj_num = atoi(optarg);
			if (_g_obj_num < 1)
				return -1;
			break;

		case 'o':
			_g_obj_size = atoi(optarg);
			if (_g_obj_size < 1)
				return -1;
			break;

		case 'l':
			_g_loop_times = atoi(optarg);
			if (_g_loop_times < 1)
				return -1;
			break;

		case 'h':
			return -1;

		case ':':
			printf("option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("unkowned option %c\n", optopt);
			return -1;
		}
	}

	if (optind < argc) {
		return -1;
	}

	printf("thread number is %d, loop times is %d, "
	       "object number is %d, object size is %d\n",
	       _g_thread_num, _g_loop_times, _g_obj_num, _g_obj_size);

	return 0;
}

static int 
_initiate(void)
{
	if (_g_thread_num > 1) {
		_g_mempool = mempool_alloc(_g_obj_size, 3, 1);
	}
	else {
		_g_mempool = mempool_alloc(_g_obj_size, 3, 0);
	}

	if (!_g_mempool)
		return -1;
 
	mempool_print(_g_mempool);

	return 0;
}

static void 
_release(void)
{
	if (_g_mempool) {
		mempool_print(_g_mempool);
		mempool_free(_g_mempool);
		_g_mempool = NULL;
	}
}

static void *
_thread_run(void *arg)
{
	int i;
	int j;
	char *ptr;

	for (i = 0; i < _g_loop_times; i++) {
		ptr = mempool_get(_g_mempool);
		if (!ptr) {
			printf("mempool_get failed\n");
			continue;
		}

		mempool_print(_g_mempool);

		for (j = 0; j < _g_obj_size; j++) {
			ptr[j] = j * i & 0xFF;
		}

//		mempool_put1(_g_mempool, ptr);
		mempool_put(ptr);

//		mempool_print(_g_mempool);
	}

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

	if (_g_thread_num > 1) {
		for (i = 0; i < _g_thread_num; i++) {
			if (pthread_create(&(_g_thread_ids[i]), NULL, 
					   _thread_run, NULL)) 
			{
				printf("create thread failed\n");
			}

			printf("create thread %lu\n", _g_thread_ids[i]);
		}

		sleep(1);

		for (i = 0; i < _g_thread_num; i++) {
			if (pthread_join(_g_thread_ids[i], NULL)) {
				printf("wait thread %lu exit failed\n", 
				       _g_thread_ids[i]);
			}
			
			printf("stop thread %lu\n", _g_thread_ids[i]);
		}
	}
	else {
		_thread_run(NULL);
	}

	_release();

	return 0;
}


