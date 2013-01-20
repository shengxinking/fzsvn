/**
 *	@file	sysutil.h
 *
 *	@brief	some linux system utilities APIs, include CPU/Memory.
 *
 *	@date	2012-07-23
 *	@author	Forrest.Zhang.
 */

#ifndef	FZ_SYSUTIL_H
#define	FZ_SYSUTIL_H

/**
 *	Get the CPU usage, if have many CPU, it's all cpus usage.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
cpu_usage(void);

/**
 *	Get each CPU usage and stored in @cpus, if ncpu > CPU nunber,
 *	only return system cpu usage number.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
cpus_usage(int *cpus, int ncpu);

/**
 *	Get the CPU number in runing system.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
cpu_number(void);

/**
 *	Get the CPU frequence. using HZ as meter.
 *	Return > 0 if success, -1 on error
 */
extern int 
cpu_freq(void);



/**
 *	Get the total memory(MB) in system.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
mem_total(void);

/**
 *	Get the freed memory(MB) in system.
 *
 *	Return > 0 if success, -1 on error
 */
extern int 
mem_freed(void);

/**
 *	Get the used memory(MB) in system.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
mem_used(void);

/**
 *	Get the memory usage in system
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
mem_usage(void);

/**
 *	Set the process name to @newname
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
proc_rename(const char *newname, char **argv);

/**
 *	Return the first pid of process which name is @procname
 *
 *	Return > 0 if success, -1 if not exist.
 */
extern pid_t 
proc_find(const char *procname);

/**
 *	Return the RSS memory size(KB) of running process.
 *
 *	
 */
extern u_int64_t 
proc_rss(void);

/**
 *	Check the process is exist or not.
 *
 *	return 0 if exist, -1 if not exist
 */
extern int 
proc_exist(pid_t pid);

#define	PID_FILE_NAMELEN	128

/**
 * 	Check a pid file /var/run/@pname.pid exist or not.
 *
 * 	Return 1 if exist, 0 not exist.
 */
extern int 
pid_file_exist(const char *pname);

/**
 *	Create a new pidfile on /var/run/@pname.pid 	
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
pid_file_new(const char *pname, pid_t pid);

/**
 *	Read the pid file /var/run/@pname.pid and return 	
 *	the pid value.
 *
 * 	Return > 0 is the pid, -1 on error.
 */
extern pid_t 
pid_file_read(const char *pname);

/**
 * 	Remove a old pidfile on /var/run/@pname.pid
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
pid_file_del(const char *pname);



#endif	/* end of FZ_SYSUTIL_H */
