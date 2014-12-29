/**
 *	@file	dbg_segfault.h
 *
 *	@brief	the segfault handler for program's SEGV.
 */

typedef void (*sighandler_t)(int signo);

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
dbg_segfault(const char *pname, sighandler_t handler);



