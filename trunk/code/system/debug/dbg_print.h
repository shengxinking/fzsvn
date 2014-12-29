/**
 *	@file	dbg_print.h
 *
 *	@brief	The debug API header file for output debug message.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_DBG_PRINT_H
#define FZ_DBG_PRINT_H

#define	DBG_LINE_LEN		1024

/**
 *	Print the message on console and pts it there
 *	are enabled.
 *	
 *	No return.
 */
extern void 
dbg_print(const char *fmt, ...) __attribute((format(printf, 1, 2)));

/**
 *	Output message to console.
 *
 * 	No return.
 */
extern void 
dbg_console(const char *fmt, ...) __attribute((format(printf, 1, 2)));

/**
 *	Output message to all PTS.
 *
 * 	No return.
 */
extern void 
dbg_pts(const char *fmt, ...) __attribute((format(printf, 1, 2)));

/**
 *	Enable PTS @pts debug output
 *
 * 	Return 0 if success, -1 on error..
 */
extern int 
dbg_enable_pts(const char *pts);

/**
 *	Disable the PTS @pts debug output.
 *
 * 	Return 0 if success, -1 on error..
 */
extern int 
dbg_disable_pts(const char *pts);

/**
 *	Check the PTS @pts debug output is enabled 
 *	or not.
 *
 * 	Return 1 if enabled, 0 if not.
 */
extern int 
dbg_pts_is_enabled(const char *pts);

/**
 *	Enable console debug output
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
dbg_enable_console(void);

/**
 *	Disable console debug output.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dbg_disable_console(void);

/**
 *	Check the console debug output is 
 *	enabled or not.
 *
 *	Return 1 if enabled, 0 if not.
 */
extern int 
dbg_console_is_enabled(void);


#endif /* end of FZ_DBG_PRINT_H  */

