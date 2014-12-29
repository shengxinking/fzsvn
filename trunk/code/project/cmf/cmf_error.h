/**
 *	@file	clierror.h
 *
 *	@brief	Define all error number used in CLI.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_CLI_ERROR_H
#define FZ_CLI_ERROR_H

#define	CLI_PATH_ERROR		1
#define CLI_INVALID_PARAM	2
#define CLI_NAME_TOOLONG	3
#define CLI_MALLOC_FAILED	4

/**
 *	The global error number
 */
extern int	cli_error_number;


#endif /* end of FZ_CLI_ERROR_H  */

