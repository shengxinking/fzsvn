/*
 *	binarysearch_test.c:	binary search test program
 *
 *	author:			forrest.zhang
 */

#include <stdio.h>
#include <time.h>
#include <getopt.h>
#include <stdlib.h>

#include "binarysearch.h"

static int *g_data;
static int g_data_num = 5;
static int g_data_val = 3;

static void _usage(void)
{
	printf("binarysearch_test <options>\n");
	printf("-n\tnumber of element\n");
	printf("-v\tvalue need find\n");
	printf("-h\thelp infomation\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char optstr[] = ":n:v:h";
	char c;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'n':
			g_data_num = atoi(optarg);
			if (g_data_num < 1)
				return -1;
			break;

		case 'v':
			g_data_val = atoi(optarg);
			if (g_data_val < 1)
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

static int _release(void)
{
	if (g_data)
		free(g_data);
	return 0;
}

static int _int_cmp(const void *a, const void *b)
{
	if (!a && b)
		return -1;
	else if (a && !b)
		return 1;
	else if (!a && !b)
		return 0;
	else
		return (*(int *)a - *(int *)b);
}

int main(int argc, char **argv)
{
	int *val;
	int i;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init())
		return -1;

	qsort(g_data, g_data_num, sizeof(int), _int_cmp);
	printf("data is:\n");
	for (i = 0; i < g_data_num; i++)
		printf("%8d, ", g_data[i]);
	printf("\n");

	val = binarysearch(g_data, sizeof(int), g_data_num, _int_cmp, &g_data_val);

	if (val)
		printf("find data %d\n", *val);
	else
		printf("not founded\n");

	if (_release())
		return -1;

	return 0;
}

