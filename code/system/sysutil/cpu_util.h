/**
 *	@file	cpu_util.h
 *
 *	@brief	APIs get CPU information.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_CPU_UTIL_H
#define FZ_CPU_UTIL_H

/**
 *	Get the total CPU usage, if have multi-CPUs, 
 *	it's all cpus usage.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
cpu_get_total_usage(void);

/**
 *	Get each CPU usage and stored in @cpus, if ncpu > CPU number,
 *	only return real cpu usage.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
cpu_get_usage(int *cpus, int ncpu);

/**
 *	Get the CPU number in runing system.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
cpu_get_number(void);

/**
 *	Get the CPU frequence. using HZ as meter.
 *
 *	Return > 0 if success, -1 on error
 */
extern int 
cpu_get_frequence(void);

#endif /* end of FZ_CPU_UTIL_H  */


