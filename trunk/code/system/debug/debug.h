/**
 *	@file	debug.h
 *
 *	@brief	provide APIs to debug program abnormal exit SEGV, 
 *		and log message to log file.
 *
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_DEBUG_H
#define FZ_DEBUG_H

#define	DBG_LOG_FILE	"/var/log/crash.log"	/* the log file name */
#define	DBG_LOG_NUM	8096		/* the max log record */
#define	DBG_LOG_LINELEN	1024		/* the record line length */


/**
 *	Clear the crash log.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dbg_clear_log(void);


/**
 *	Read the  log
 *
 */
extern int 
dbg_read_log(int index, char *buf, size_t len);

/**
 *
 *
 */
extern int 
dbg_write_log(const char *record, size_t len);

/**
 *	log the segfault register value and call-stack 
 *	into log file(@DBG_LOG_FILE). The @pname is 
 *	program's name, the @sig_handler is the signal 
 *	hanlder function which will called before log
 *	the call-stack.
 *	
 *	Return 0 if register segfault handler success.
 *	-1 on error.
 */
extern int 
dbg_segfault(const char *pname, sig_handler_t handler);


#endif /* end of FZ_DEBUG_H  */


