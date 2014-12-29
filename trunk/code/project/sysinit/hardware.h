/**
 *	@file	hardware.h
 *
 *	@brief	The hardware APIs for init
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2010-08-31
 */

#ifndef FZ_HARDWARE_H
#define FZ_HARDWARE_H

/**
 *	Init XMLCP card when system startup
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
hw_init_xmlcp(void);


/**
 *	Init CP7 bypass function when system startup
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
hw_init_bypass(void);


#endif /* end of FZ_HARDWARE_H  */

