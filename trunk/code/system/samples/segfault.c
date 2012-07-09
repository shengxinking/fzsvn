/*
 *	@file	segfault.c
 *
 *	@brief	using glibc backtrace and backtrace_symbols to print the
 *		call-stack when it recv a SEGSEG
 *
 *
 *	@author	Forrest.zhang
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>


static void _usage(void)
{

	printf("segfault <options>\n");
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



void _print_trace(void)
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


void _sig_segv(int signo)
{
	
	if (signo == SIGSEGV) {
		_print_trace();
		exit(0);
	}

}

int _dummy_function1(void)
{
	void *ptr = NULL;

	strcpy(ptr, "segfault");

	return 0;
}

int _dummy_function(void)
{
	return _dummy_function1();
}

static int _init(void)
{
	struct sigaction act;

	act.sa_handler = _sig_segv;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_restorer = NULL;
	if (sigaction(SIGSEGV, &act, NULL)) {
		printf("sigaction error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}


int main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init()) {
		return -1;
	}

	_dummy_function();

	
	return 0;
}
