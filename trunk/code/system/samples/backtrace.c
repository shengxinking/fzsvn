/*
 *	@file	backtrace.c
 *
 *	@brief	it print the stack of calling function.
 *		NOTE: it'll only print non-static functions, you need
 *		define function non-static if you want trace.
 *
 *	@author	Forrest.zhang
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <getopt.h>

static void _usage(void)
{

	printf("backtrace <options>\n");
	printf("\t-h\tshow help message\n");
}

static int _parse_cmd(int argc, char **argv)
{
	char optstr[] = ":h"; 
	char c;

	opterr = 0;
	while ( (c = getopt(argc, argv, optstr)) != -1) {
		
		switch (c) {

		case 'h':
			return -1;

		case ':':
			printf("option %c missing argument\n", c);
			return -1;

		case '?':
			printf("unkowned option %c\n", c);
			return -1;
		}
	}

	if (argc - optind > 0) 
		return -1;

	return 0;
}

static void _print_trace(void)
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);

	printf("Obtained %zd stack frames.\n", size);

	for (i = 0; i < size; i++)
		printf("%i %s\n", i, strings[i]);


	free(strings);
}

void _dummy_function(void)
{
	_print_trace();
}

int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	_dummy_function();

	return 0;
}
