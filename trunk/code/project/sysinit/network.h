/**
 *	@file	network.h
 *
 *	@brief	the network APIs for init
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2010-08-31
 */

#ifndef FZ_NETWORK_H
#define FZ_NETWORK_H

/**
 *	Change the network interface name.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
nw_chname(void);


/**
 *	Set the kernel tcp parameter.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
nw_set_tcp(void);


/**
 *	Set the lo device IP.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
nw_set_lo(void);


#endif /* end of FZ_NETWORK_H  */

