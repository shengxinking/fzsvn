/*
 *
 *
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>

#include "shmpool.h"

static int g_shmkey = 0x12345678;
static char g_cmd;
static int g_pos;

static void _usage(void)
{
	printf("shmpool_test <options>\n");
	printf("\t-n\tcreate a new shmpool\n");
	printf("\t-d\tdelete a shmpool\n");
	printf("\t-g\tget a shmpool\n");
	printf("\t-h\tshow help message\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char c;
	char optstr[] = ":ndsgp:h";

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {

		case 'n':
			g_cmd = 'n';
			break;
		case 'd':
			g_cmd = 'd';
			break;
		case 's':
			g_cmd = 's';
			break;
		case 'g':
			g_cmd = 'g';
			break;
		case 'p':
			g_cmd = 'p';
			g_pos = atoi(optarg);
			break;
		case 'h':
			return -1;
		case ':':
			printf("option %c missing argument\n", optopt);
			return -1;
		case '?':
			printf("unknow option %c\n", optopt);
			return -1;
		}
	}

	if (optind < argc)
		return -1;
		
	return 0;
}

static int _init()
{
	return 0;
}

static int _release()
{
	return 0;
}

int main(int argc, char **argv)
{
	shmpool_t	*pool = NULL;
	shmpool_node_t	*node = NULL;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init())
		return -1;

	switch (g_cmd) {
		case 'n':
			pool = shmpool_alloc(g_shmkey, 8, 8);
			shmpool_print(pool);
			break;
		case 'd':
			pool = shmpool_attach(g_shmkey);
			shmpool_free(pool);
			break;
		case 's':
			pool = shmpool_attach(g_shmkey);
			shmpool_print(pool);
			break;
		case 'g':
			pool = shmpool_attach(g_shmkey);
			shmpool_print(pool);
			node = shmpool_get(pool);
			if (node) {
				printf("get a node, size %d, len %d, next %d, pos is %d\n", 
					node->size, node->len, node->next, shmpool_idx(pool, node));
				shmpool_print(pool);
			}
			else {
				printf("shmpool_get error\n");
			}
			break;
		case 'p':
			pool = shmpool_attach(g_shmkey);
			shmpool_print(pool);
			node = shmpool_ptr(pool, g_pos);
			if (node) {
				if (shmpool_put(pool, node)) {
					printf("put node error\n");
					return -1;
				}
				shmpool_print(pool);
			}
			else {
				printf("invalid pos %d\n", g_pos);
			}
			break;
	}

	_release();

	return 0;
}

