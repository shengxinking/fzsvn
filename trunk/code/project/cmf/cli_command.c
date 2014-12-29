/**
 *	@file	command.c
 *
 *	@brief	CLI client command implement.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2009-01-04
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "cli_command.h"

static int 
_cli_cmd_config(cli_context_t *cxt)
{
	return 0;
}


static int 
_cli_cmd_exit(cli_context_t *ctx)
{
	return 0;
}

static int 
_cli_cmd_edit(cli_context_t *cxt)
{
	return 0;
}


static int 
_cli_cmd_set(cli_context_t *ctx)
{
	return 0;
}

static int 
_cli_cmd_unset(cli_context_t *ctx)
{
	return 0;
}


static int
_cli_cmd_get(cli_context_t *ctx)
{
	return 0;
}


static int 
_cli_cmd_show(cli_context_t *ctx)
{
	return 0;
}


extern int 
_cli_cmd_quit(cli_context_t *ctx)
{
	return 0;
}


cli_command_t _root_commands[] = {
	{"config", "config object", 0, _cli_cmd_config},
	{"get", "get config", 0, _cli_cmd_get},
	{"show", "show config command", 0, _cli_cmd_show},
	{"exit", "exit CLI", 0, _cli_cmd_exit},
	{"", "", 0, NULL},
};


cli_command_t _single_commands[] = {
	{"set", "set a attribute value", 0, _cli_cmd_set},
	{"unset", "unset a attribute value", 0, _cli_cmd_unset},
	{"get", "get a attribute value", 0, _cli_cmd_get},
	{"show", "show a attribute value", 0, _cli_cmd_show},
	{"", "", 0, NULL}
};


cli_command_t _table_commands[] = {
	{"edit", "Edit a object", 0, _cli_cmd_edit},
	{"", "", 0, NULL},
};


static int 
_cli_run_root_command(cli_context_t *ctx, const char *cmd, char *errbuf, int len)
{
	return 0;
}


/**
 *	Process CLI command, if unsuccess, the error message is
 *	stored in @errbuf.
 *
 *	Return 0 if execute command success, -1 on error.
 */
int
cli_run_command(cli_context_t *ctx, const char *cmd, char *errbuf, int len)
{
	

	if (!ctx || !cmd)
		return -1;

	cli_split_command();

	return 0;
}




