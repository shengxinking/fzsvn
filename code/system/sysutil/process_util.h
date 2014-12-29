/**
 *	@file	process_util.h
 *
 *	@brief	APIs get process information.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	
 */

#ifndef FZ_PROCESS_UTIL_H
#define FZ_PROCESS_UTIL_H

/**
 *	Set the process name to @newname
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
process_rename(const char *newname, char **argv);

/**
 *	Return the pids of all process which name is @procname
 *
 *	Return > 0 if success, -1 if not exist.
 */
extern pid_t * 
process_find(const char *pname);

/**
 *	Return the RSS memory size(KB) of running process.
 *
 *	
 */
extern u_int64_t 
process_get_rss(void);

/**
 *	Check the process is exist or not.
 *
 *	return 0 if exist, -1 if not exist
 */
extern int 
process_is_exist(pid_t pid);

/**
 *	Bind process @pid to CPU @cpu.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
process_bind_cpu(pid_t pid, int cpu);

#endif /* end of FZ_PROCESS_UTIL_H  */


