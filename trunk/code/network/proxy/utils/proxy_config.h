/**
 *	@file	proxy_config.h
 *
 *	@brief	proxy config file APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PROXY_CONFIG_H
#define FZ_PROXY_CONFIG_H

#include "proxy.h"

/**
 *	Load config from config file, save config into @py.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cfg_load_file(proxy_t *py, const char *file);

/**
 *	Write config to config file.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cfg_write_file(proxy_t *py, const char *file);


#endif /* end of FZ_PROXY_CONFIG_H */

