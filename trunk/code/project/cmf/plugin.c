/**
 *	@file	plugin.c
 *
 *	@brief	Run all CLI register function to add all CLI object to 
 *		tree @g_cli_root.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2008-01-08
 */

#include "debug.h"
#include "plugin.h"



/**
 *	Run all plugin register function, to add all CLI object
 *	to tree.
 */
extern int 
cli_plugin_init(void)
{
	int ret = 0;

	/* register system.global */
	ret = cli_register_system_global();
	if (ret < 0) {
		CLI_ERR("register system.global error\n");
		return -1;
	}

	return 0;
}


