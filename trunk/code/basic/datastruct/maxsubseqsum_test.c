/*
 *	maxsubseqsum_test.c:	test program for max subsequence sum algthrithm
 *
 * 	author:			forrest.zhang
 */

#include "maxsubseqsum.h"
#include <stdio.h>
#include <string.h>

static int _int_add(void *s, const void *i, const void *j)
{
	if (!s || !i || !j)
		return -1;

	*(int *)s = *(int *)i + *(int *)j;

	return 0;
}

static int _int_cmp(const void *i, const void *j)
{
	if (!i && !j)
		return 0;

	if (!i && j)
		return 0 - *(int*)j;

	if (i && !j)
		return *(int *)i;
}

static void _usage(void)
{
}

static int _parse_cmd(int argc, char **argv)
{
	return 0;
}

static int _init(void)
{
}

int main(int argc, char **argv)
{
	int i;

	return 0;
}

