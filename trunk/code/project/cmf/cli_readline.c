/**
 *	@file	cli.c
 *
 *	@brief	CLI main entry file.
 *	
 *	@date	2009-01-04
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "cli.h"
#include "object.h"
#include "command.h"


/**
 *	the 'TAB' character is do real complete in CLI. It'll rotato
 *	all possible words if more than one complete words, or complete
 *	it if only one complete word and append a white space.
 *
 *	Alway return 0;
 */
static int 
_cli_tab_complete(int count, int key)
{
	char **names = NULL;
	
//	names = str_split(rl_line_buffer);
	if (!names)
		return 0;
	
	
	return 0;
}

/**
 *	the '?' character character is show the complete words and their 
 *	help message. and the CLI will show the character which already
 *	input.
 *
 *	always return 0;
 */
static int 
_cli_question_help(int count, int key)
{
	char **names = NULL;

//	names = _cli_str_split(rl_line_buffer);

//	obj = cli_find_object(names);

	return 0;
}

/**
 *	Initiate CLI environment.
 *
 *	Return 0 if success, -1 on error.
 */
int 
cli_init(cli_context_t *ctx, int issvr)
{
	if (!ctx)
		return -1;

	memset(ctx, 0, sizeof(cli_context_t));

	if (issvr) {
		
	}
	else {
		/** 
		 * set readline environment:
		 * Prompt, TAB complete, ? show help message
		 */
		strcpy(ctx->prompt, "CLI > ");
		rl_bind_key('\t', _cli_tab_complete);
		rl_bind_key('?', _cli_question_help);
	}

	return 0;
}

/**
 *	Free resource used in CLI
 *
 *	No return.
 */
void 
cli_free(cli_context_t *ctx)
{
	if (!ctx)
		return;

	if (ctx->buf) {
		free(ctx->buf);
		ctx->buf = NULL;
	}
}

/**
 *	Get a command from readline. Must release the buf after
 *	using.
 *
 *	Return the command line if success, NULL on error. 
 */
char * 
cli_readline(cli_context_t *ctx)
{
	char *buf = NULL;
	
	buf = readline(ctx->prompt);

	return buf;
}



