/**
 *	@file	plugin.h
 *
 *	@brief	The plugin function declared here.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2009-01-08
 */

#ifndef FZ_CLI_PLUGIN_H
#define FZ_CLI_PLUGIN_H

#ifndef CLI_NAMELEN
#define CLI_NAMELEN	31
#endif

typedef struct sys_global {
	char	hostname[CLI_NAMELEN + 1];
	int	guiport;
	int	guisport;
} sys_global_t;


/**
 *	Run all plugin register function, to add all CLI object
 *	to tree.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_plugin_init(void);


/**
 *	Clear all CLI plugins, free all memory in CLI objects.
 *
 *	No return.
 */
extern void
cli_plugin_free(void);


/* system.global */
extern int 
cli_register_system_global(void);


#endif /* end of FZ_  */

