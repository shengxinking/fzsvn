/**
 *	@file	sysstatus.h
 *
 *	@brief	get the system status, cpu information, memory information
 *		and disk information
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_SYSSTATUS_H
#define FZ_SYSSTATUS_H

typedef struct sys_cpu_usage {
	u_int32_t	utime;
	u_int32_t	ntime;
	u_int32_t	stime;
	u_int32_t	itime;
} sys_cpu_usage_t;


typedef struct sys_mem_usage {
	u_int32_t	total;
	u_int32_t	freed;
	u_int32_t	cached;
} sys_mem_usage_t;

typedef struct sys_disk_usage {
	u_int32_t	total;
	u_int32_t	freed;
	char		dname[32];
} sys_disk_usage_t;


/**
 *	Get CPU usage of system, it's get information from
 *	/proc/stat file.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sys_get_cpu_usage(sys_cpu_usage_t *usg, int cpuid, int useconds);


/**
 *	Get memory usage of system, it's get information from
 *	/proc/meminfo file.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sys_get_mem_usage(sys_mem_usage_t *usg);


/**
 *	Get disk usage of system, it'll check the disk @dname
 *	is mount or not, if it's mouted, using fstatfs(2), if
 *	@dname is not mounted, return -1.
 *
 *	Return 0 if success, -1 on error or not mounted.
 */
extern int
sys_get_disk_usage(const char *dname, sys_disk_usage_t *usg);


#endif /* end of FZ_SYSSTATUS_H  */

