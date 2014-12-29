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

static int _g_loop_count = 1;
static mempool_t *_g_mempool = NULL;

static void 
_usage(void)
{
	printf("mempool_test <options>\n");
	printf("\t-l\tloop times of get/put\n");
	printf("\t-h\tshow help message\n");
}


static int 
_parse_cmd(int argc, char **argv)
{
	char optstr[] = ":l:h";
	char c;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'l':
			_g_loop_count = atoi(optarg);
			if (_g_loop_count < 1)
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

	return 0;
}

static int 
_initiate(void)
{
	_g_mempool = mempool_create(1);
	if (!_g_mempool)
		return -1;
	return 0;
}

static void 
_release(void)
{
	if (_g_mempool)
		mempool_destroy(_g_mempool);
}

int 
main(int argc, char **argv)
{
	int i;
	int j;
	char *ptr;
	int *array;
	size_t oldsize;
	size_t newsize;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		_release();
		return -1;
	}

	for (i = 0; i < _g_loop_count; i++) {
		ptr = mempool_alloc(_g_mempool, 1024);
		ptr[1023] = 's';
		ptr = mempool_calloc(_g_mempool, 128);
		ptr[0] = 'b';

		oldsize = sizeof(int) * 10;
		array = mempool_alloc(_g_mempool, oldsize);
		for (j = 0; j < 10; j++)
			array[j] = 10 + j;
		newsize = sizeof(int) * 20;
		array = mempool_realloc(_g_mempool, array, oldsize, newsize);
		for (j = 0; j < 20; j++)
			printf("%d is %d\n", j, array[j]);

	}

	_release();

	return 0;
}


