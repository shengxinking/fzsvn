/**
 *	@file	engine.c
 *
 *	@brief	A simple engine implement, it call the openssl origin function.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2009-11-27
 */


#include <openssl/engine.h>

/** 
 *	Init ENGINE @ee's hardware and environment.
 *
 *	Return 1 if success, other value if failed.
 */
static int _ee_init(ENGINE *ee)
{
	printf("ENGINE %s is initiated!\n", ENGINE_get_id(ee));

	return 0;
}



/**
 *	Create a new engine object and add it to openssl.
 *
 *	Return the ENGINE object if success, -1 on error.
 */
ENGINE *  
ee_alloc(void)
{
	ENGINE *ee = NULL;
	const char *eid = "TEST";
	
	ENGINE_load_builtin_engines();
	e = ENGINE_new();
	if (!e) {
		printf("Can't create new empty engine\n");
		return NULL;
	}

	ENGINE_set_id(e, "TEST");
	ENGINE_set_name(e, "Test Engine");
	
	

	return ee;
}

/**
 *	Free a engine, get it out of openssl.
 *
 *	No return.
 */
void 
ee_free(ENGINE *ee)
{


}


/**
 *	Using ENGINE @ee as openssl default ENGINE.
 *
 *	Return 0 if success.
 */
int 
ee_start(ENGINE *ee)
{

	return 0;
}


/**
 *	Stop the ENGINE @ee as openssl default ENGINE.
 *
 *	No return.
 */
void 
ee_stop(ENGINE *ee)
{


}


