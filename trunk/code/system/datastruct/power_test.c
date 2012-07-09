/*
 *
 *
 *	author:		forrest.zhang
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "power.h"

static double g_x = 0;
static unsigned int g_y = 0;

static void _usage(void)
{
	printf("power_test <x> <y>\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char *c;

	if (argc != 3)
		return -1;

	g_x = strtod(argv[1], &c);
	if (*c != '\0')
		return -1;
	g_y = atoi(argv[2]);
	if (g_y < 0)
		return -1;

	return 0;
}

int main(int argc, char **argv)
{
	double ret;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	ret = power(g_x, g_y);
	printf("%f^%u is %f\n", g_x, g_y, ret);

	return 0;
}

