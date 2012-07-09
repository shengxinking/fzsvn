/**
 *	@file	datatype.h
 *
 *	@brief	define data type used in CLI
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2008-12-26
 */

#ifndef FZ_CLI_DATATYPE_H
#define FZ_CLI_DATATYPE_H

#define CLI_DTYPE_INT		0
#define CLI_DTYPE_SHORT		1
#define CLI_DTYPE_UINT		2
#define CLI_DTYPE_LONG		3
#define CLI_DTYPE_ULONG		4
#define CLI_DTYPE_ID		5
#define CLI_DTYPE_STR		6
#define CLI_DTYPE_TXT		7
#define CLI_MAX_TYPE		CLI_DTYPE_TXT


extern int 
cli_object_config(cli_object_t *obj, cli_value_t *val, const char *buf, size_t len);


extern int 
cli_config_object(cli_value_t *val, cli_object_t *obj, const char *buf, size_t len);


#endif /* end of FZ_CLI_DATATYPE_H  */


