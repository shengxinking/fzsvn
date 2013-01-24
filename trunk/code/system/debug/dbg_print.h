/**
 *	@file	dbg_print.h
 *
 *	@brief	The debug API header file
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_DBG_PRINT_H
#define FZ_DBG_PRINT_H


/**
 *	Print the message on console and pts it there
 *	are enabled.
 *	
 *	No return.
 */
extern void 
dbg_print(const char *fmt, ...);

extern void 
dbg_console(const char *fmt, ...);

extern void 
dbg_pts(const char *fmt, ...);

extern void 
dbg_enable_pts(int pts);

extern void 
dbg_disable_pts(int pts);

extern void 
dbg_enable_console(void);

extern void 
dbg_disable_console(void);



#endif /* end of FZ_DBG_PRINT_H  */

