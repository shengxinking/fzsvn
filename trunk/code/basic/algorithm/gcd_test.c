/*
 *	gcd_test.c:	Euclid gcd algorithm test program
 *
 *	author:		forrest.zhang
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "gcd.h"

static unsigned long g_m = 0;
static unsigned long g_n = 0;

static void _usage(void)
{
	printf("gcd_test <m> <n>\n");
}

static int _parse_cmd(int argc, char **argv)
{
	if (argc != 3)
		return -1;

	g_m = atoi(argv[1]);
	if (g_m < 1)
		return -1;

	g_n = atoi(argv[2]);
	if (g_n < 1)
		return -1;

	return 0;
}

int main(int argc, char **argv)
{
	unsigned long g = 0;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	g = gcd(g_m, g_n);
	printf("%lu and %lu gcd is %lu\n", g_m, g_n, g);

	return 0;
}

