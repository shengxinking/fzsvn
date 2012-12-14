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
fz_cpu_usage(void);

/**
 *	Get each CPU usage and stored in @cpus, if ncpu > CPU nunber,
 *	only return system cpu usage number.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
fz_cpus_usage(int *cpus, int ncpu);

/**
 *	Get the CPU number in runing system.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
fz_cpu_number(void);

/**
 *	Get the CPU frequence. using HZ as meter.
 *	Return > 0 if success, -1 on error
 */
extern int 
fz_cpu_freq(void);



/**
 *	Get the total memory(MB) in system.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
fz_mem_total(void);

/**
 *	Get the freed memory(MB) in system.
 *
 *	Return > 0 if success, -1 on error
 */
extern int 
fz_mem_freed(void);

/**
 *	Get the used memory(MB) in system.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
fz_mem_used(void);

/**
 *	Get the memory usage in system
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
fz_mem_usage(void);



/**
 *	Set the process name to @newname
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
fz_proc_rename(const char *newname, char **argv);

/**
 *	Return the pid of process which name is @procname
 *
 *	Return > 0 if success, -1 if not exist.
 */
extern pid_t 
fz_proc_find(const char *procname);

/**
 *	Return the RSS memory size(KB) of running process.
 *
 *
 */
extern u_int64_t 
fz_proc_rss(void);

/**
 *	Check the process is exist or not.
 *
 *	return 0 if exist, -1 if not exist
 */
extern int 
fz_proc_exist(pid_t pid);

/**
 *	Create a new pidfile on /var/run/@pname.pid 	
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
fz_pidf_new(const char *pname);

/**
 * 	Remove a old pidfile on /var/run/@pname.pid
 *
 * 	Return 0 if success, -1 on error.
 */
extern int 
fz_pidf_del(const char *pname);

/**
 * 	Check a program is exist by pidfile /var/run/@pname.pid.
 *
 * 	Return 1 if exist, 0 not found.
 */
extern int 
fz_pidf_exist(const char *pname);


#endif	/* end of FZ_SYSUTIL_H */
