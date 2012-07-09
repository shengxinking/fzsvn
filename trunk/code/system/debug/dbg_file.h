/**
 *	@file	dbg_file
 *
 *	@brief	log debug information into file
 *	
 *	@author	
 *
 *	@date	2010-07-19
 */

#ifndef DBG_FILE_H
#define DBG_FILE_H

/* max line length in file */
#define DBG_LINE_MAX		1024
/* max progrom name length */
#define DBG_PROGNAME_MAX	64

/* debug file path */
#define DBG_FILE_DIR		"/var/log/debug"
#define DBG_ABNORMAL_FILE	"/var/log/debug/abnormal"
#define DBG_INFLOOP_FILE	"/var/log/debug/infloop"
#define DBG_DEADLOCK_FILE	"/var/log/debug/deadlock"

/**
 *	init the dbg_file APIs.
 *
 *	No return.
 */
extern void 
dbg_file_init(const char *progname);

/**
 *	Log the abnormal information into file.
 *
 *	No return.
 */
extern void 
dbg_abnormal(const char *fmt, ...);


/**
 *	Log the infinite loop message into file.
 *
 *	No return.
 */
extern void 
dbg_infloop(const char *fmt, ...);

/**
 *	Log the dead lock into file.
 *
 *	No return.
 */
extern void 
dbg_deadlock(const char *fmt, ...);


/**
 *	Release resource alloc by dbg_file_init
 *
 *	No return.
 */
void 
dbg_file_release(void);

#endif /* end of DBG_FILE_H  */

