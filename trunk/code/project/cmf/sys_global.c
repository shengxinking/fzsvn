/**
 *	@file	system.c
 *
 *	@brief	Add system plugin.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-01-08
 */

#include <unistd.h>

#include "object.h"
#include "plugin.h"

/**
 *	Add system.global to CLI.
 *
 *	Return 0 if success, -1 on error.
 */
int cli_register_system_global(void)
{
	void *obj = NULL;
	void *attr = NULL;

	obj = cli_object_add("system", "global", "global infomation", 
			     CLI_OBJ_UNI, 1, CLI_OBJ_GRP_RDWR,  0, 
			     sizeof(sys_global_t));
	if (!obj) {
		return -1;
	}

	attr = cli_attr_add(obj, "hostname", "Host name", 
			    CLI_DTYPE_STR, CLI_NAMELEN + 1, 
			    CLI_OFFSET(sys_global_t, hostname));
	if (!attr)
		return -1;
	
	
	attr = cli_attr_add(obj, "gui-port", "GUI http port", 
			    CLI_DTYPE_INT, sizeof(int), 
			    CLI_OFFSET(sys_global_t, guiport));
	if (!attr)
		return -1;

	attr = cli_attr_add(obj, "gui-sport", "GUI https port", 
			    CLI_DTYPE_INT, sizeof(int), 
			    CLI_OFFSET(sys_global_t, guisport));
	if (!attr)
		return -1;

	return 0;
}






