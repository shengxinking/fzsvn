/**
 *	@file	dbg_log.h
 *
 *	@brief	provide APIs to debug program abnormal exit SEGV, 
 *		and log message to log file.
 *
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_DBG_LOG_H
#define FZ_DBG_LOG_H

#include <sys/types.h>

#define	DBG_LOG_LEN		1024

enum {
	DBG_LOG_CRASH,
	DBG_LOG_KERNEL,
	DBG_LOG_CONSOLE,
	DBG_LOG_MAX,
};

#define	DBG_LOG_CRASH(fmt, args...)		\
	dbg_log(DBG_LOG_CRASH, fmt, ##args)

#define	DBG_LOG_KERNEL(fmt, args...)		\
	dbg_log(DBG_LOG_KERNEL, fmt, ##args)

#define	DBG_LOG_CONSOLE(fmt, args...)		\
	dbg_log(DBG_LOG_CONSOLE, fmt, ##args)


/**
 *	Init log, the log type is @type.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dbg_init_log(int type);

/**
 *	Read the log from log file, the log index 
 *	is @index
 *
 * 	Return log length >0 if success. -1 on 
 * 	error. 0 if not found.
 */
extern int 
dbg_log_read(int type, int index, char *buf, size_t len);

/**
 *	Write log into log file, the log type is @type.
 *	log string in @buf, length is @len.	
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
dbg_log_write(int type, const char *buf, size_t len);

/**
 *	Clear the crash log.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dbg_log_clear(int type);

/**
 *	Log a message like printf() format.
 *
 *	No return.
 */
extern void
dbg_log(int type, const char *fmt, ...) __attribute__((format (printf, 2, 3)));;

#endif /* end of FZ_DEBUG_H  */


