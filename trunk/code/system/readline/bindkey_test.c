/**
 *	@file	bindkey_test.c
 *
 *	@brief	Test readline's bindkey, it's another way of completion.
 *	
 *	@date	2009-01-10
 */


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>

#include <readline/readline.h>
#include <readline/history.h>

static int _tab_key_func(int start, int end);
static int _qmk_key_func(int start, int end);


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
	rl_bind_key('\t', _tab_key_func);
	rl_bind_key('?', _qmk_key_func);
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
}

static int 
_tab_key_func(int count, int key)
{
//	printf("rl_line_buffer: %s\n", rl_line_buffer);

	rl_extend_line_buffer(5);
	
	rl_line_buffer[rl_end++] = 'T';
	rl_line_buffer[rl_end++] = 'A';
	rl_line_buffer[rl_end++] = 'B';

//	printf("point is %d\n", rl_point);
//	rl_point += 3;

//	rl_done = 1;

	return 0;
}

static int 
_qmk_key_func(int count, int key)
{
//	printf("rl_line_buffer: %s\n", rl_line_buffer);

	rl_extend_line_buffer(5);

	rl_line_buffer[rl_end++] = '?';
	
//	rl_point++;

	rl_done = 1;

	return 0;
}

static void 
_do_loop(void)
{
	char *buf = NULL;

	while (1) {
		if (buf) {
			free(buf);
			buf = NULL;
		}

		buf = readline("bk: ");
		if (!buf) {
			printf("readline end\n");
			break;
		}

		printf("%s\n", buf);
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



