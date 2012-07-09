/**
 *	@file	command.h
 *
 *	@brief	CLI command declare.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2009-01-04
 */

#ifndef FZ_CLI_COMMAND_H
#define FZ_CLI_COMMAND_H

#include "cli_object.h"


#define CLI_CMDLEN	4095		/* CLI single command length */


/**
 *	CLI function type
 */
typedef int (*cli_config_func)(void *ctx, int when, void *val);
typedef int (*cli_end_func)(void *ctx, int when, void *val);
typedef int (*cli_abort_func)(void *ctx, int when, void *val);
typedef int (*cli_exit_func)(void *ctx, int when, void *val);
typedef int (*cli_edit_func)(void *ctx, int when, void *val);
typedef int (*cli_set_func)(void *ctx, int when, void *val);
typedef int (*cli_unset_func)(void *ctx, int when, void *val);
typedef int (*cli_parse_func)(void *ctx, int when, void *val);
typedef int (*cli_get_func)(void *ctx, int when, void *val);
typedef int (*cli_del_func)(void *ctx, int when, void *val);
typedef int (*cli_rename_func)(void *ctx, int when, void *val);
typedef int (*cli_show_func)(void *ctx, int when, void *val);
typedef int (*cli_print_func)(void *ctx, int when, void *val);
typedef int (*cli_delall_func)(void *ctx, int when, void *val);


/**
 *	CLI commands.
 */
typedef struct cli_command {
	char		cmd[CLI_CMDLEN + 1];	/* the command */
	char		comment[CLI_COMMLEN + 1]; /* the comment */
	uint32		flags;			/* flags */
	cli_cmdfunc_t	func;			/* the command function */
} cli_command_t;


/**
 *	Process CLI command
 *
 *	Return 0 if execute command success, -1 on error.
 */
extern int
cli_run_command(cli_context_t *ctx, const char *cmd);


#endif /* end of FZ_COMMAND_H  */

