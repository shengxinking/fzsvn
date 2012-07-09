/**
 *	@file	engine.h
 *
 *	@brief	the simple engine APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2009-11-26
 */

#ifndef FZ_ENGINE_H
#define FZ_ENGINE_H

#include <openssl/engine.h>

/**
 *	Create a new engine object and add it to openssl.
 *
 *	Return the ENGINE object if success, -1 on error.
 */
extern ENGINE *  
ee_alloc(void);

/**
 *	Free a engine, get it out of openssl.
 *
 *	No return.
 */
extern void 
ee_free(ENGINE *ee);


/**
 *	Using ENGINE @ee as openssl default ENGINE.
 *
 *	Return 0 if success.
 */
extern int 
ee_start(ENGINE *ee);

/**
 *	Stop the ENGINE @ee as openssl default ENGINE.
 *
 *	No return.
 */
extern void 
ee_stop(ENGINE *ee);

#endif /* end of FZ_ENGINE_H  */

