/**
 *	@file	cli_test.c
 *	
 *	@brief	The CLI client test program.
 *
 *	@date	2009-01-13
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <readline/readline.h>

#include "cli.h"
#include "plugin.h"
#include "command.h"

static cli_context_t g_clictx;

/**
 *	Show help message	
 *	
 *	No return.
 */
static void 
_usage(void)
{
}

/**
 *	Parse command line argument.	
 *
 * 	Return 0 if parse success, -1 on error.
 */
static int 
_parse_cmd(int argc, char **argv)
{
	char opt;
	char optstr[] = ":h";
	
	opterr = 0;
	while ( (opt = getopt(argc, argv, optstr)) != -1) {
		
		switch (opt) {
			
		case 'h':
			return -1;

		case ':':
			printf("Option %c missing argument\n", optopt);
			return -1;

		case '?':
			printf("Unknowed option %c\n", optopt);
			return -1;
		}
	}

	if (argc != optind)
		return -1;

	return 0;
}

/**
 *	Init some global resource used in program.	
 *
 * 	Return 0 if success, -1 on error.
 */
static int 
_initiate(void)
{
	cli_init(&g_clictx);

	cli_plugin_init();

	return 0;
}


/**
 *	Release global resource alloced by _initiate().	
 *
 * 	No Return.
 */
static void 
_release(void)
{
	cli_free(&g_clictx);
}

static void
_do_loop(void)
{
	char *buf = NULL;

	while (!g_clictx.stop) {

		if (buf) {
			free(buf);
			buf = NULL;
		}

		buf = cli_readline(&g_clictx);
		if (buf == NULL) {
			break;
		}

		cli_run_command(&g_clictx, buf);
	}
}


/**
 *	The main entry of program.	
 *
 * 	Return 0 if success, other value on error.
 */
int 
main(int argc, char **argv)
{
	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_initiate()) {
		return -1;
	}

	_do_loop();

	_release();

	return 0;
}





