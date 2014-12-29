/**
 *	@file	pidfile_util.h
 *
 *	@brief	APIs for pidfile.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_PIDFILE_UTIL_H
#define FZ_PIDFILE_UTIL_H

#define	PIDFILE_LEN	128

/**
 * 	Check a pid file /var/run/@pname.pid exist or not.
 *
 * 	Return 1 if exist, 0 not exist.
 */
extern int 
pidfile_is_exist(const char *pname);

/**
 *	Read the pid file /var/run/@pname.pid and return 	
 *	the pid value.
 *
 * 	Return > 0 is the pid, -1 on error.
 */
extern pid_t 
pidfile_get_pid(const char *pname);

/**
 *	Create a new pidfile on /var/run/@pname.pid 	
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
pidfile_new(const char *pname, pid_t pid);

/**
 * 	Remove a old pidfile on /var/run/@pname.pid
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
pidfile_del(const char *pname);


#endif /* end of FZ_PIDFILE_UTIL_H */


