/*
 *	kth_test.c:	test kth APIs
 *
 *	author:		forrest.zhang
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "kth.h"

static int *g_data = NULL;
static int g_data_num = 0;
static int g_kth = 0;
static int g_begin = 0;
static int g_end = 0;

static void _usage(void)
{
	printf("kth_test <options>\n");
	printf("-n\tnumber of data\n");
	printf("-k\tk number\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char c;
	char optstr[] = ":n:k:h";
	
	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'n':
			g_data_num = atoi(optarg);
			if (g_data_num <= 0)
				return -1;
			break;

		case 'k':
			g_kth = atoi(optarg);
			if (g_kth <= 0)
				return -1;
			break;

		case 'h':
			return -1;

		case ':':
			printf("option %c need argument\n", optopt);
			return -1;
		
		case '?':
			printf("unkowned option %c\n", optopt);
			return -1;
		}
	}

	if (optind < argc)
		return -1;

	if (g_kth > g_data_num)
		return -1;

	if (g_data_num <= 0)
		return -1;

	return 0;
}

static int _init(void)
{
	int i;

	g_data = malloc(sizeof(int) * g_data_num);
	if (!g_data)
		return -1;

	srand(time(NULL));
	for (i = 0; i < g_data_num; i++)
		g_data[i] = rand() % g_data_num;

	return 0;
}

static int _fini(void)
{
	if (g_data)
		free(g_data);

	return 0;
}

static void _print_data(void)
{
	int i;

	for (i = 0; i < g_data_num; i++) {
		printf("%8d ", g_data[i]);
		if ((i + 1) % 8 == 0)
			printf("\n");
	}
	printf("\n");
}

static int _int_cmp(const void *i, const void *j)
{
	assert(i);
	assert(j);

	return (*(int *)i - *(int *)j);
}

int main(int argc, char **argv)
{
	int kth;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init())
		return -1;

	g_begin = time(NULL);	
//	kth_array(g_data, g_data_num, sizeof(int), g_kth, _int_cmp, &kth);
	g_end = time(NULL);
	printf("kth(karr) is %8d, spend %8d second\n", kth, g_end - g_begin);
	g_begin = time(NULL);
	kth_sort(g_data, g_data_num, sizeof(int), g_kth, _int_cmp, &kth);
	g_end = time(NULL);
	printf("kth(sort) is %8d, spend %8d second\n", kth, g_end - g_begin);

	_fini();

	return 0;
}

