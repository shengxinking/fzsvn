/**
 *	@file	mem_util.h
 *
 *	@brief	APIs get memory information.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_MEM_UTIL
#define FZ_MEM_UTIL

/**
 *	Get the total memory(MB) in system.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
mem_get_total(void);

/**
 *	Get the freed memory(MB) in system.
 *
 *	Return > 0 if success, -1 on error
 */
extern int 
mem_get_freed(void);

/**
 *	Get the used memory(MB) in system.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
mem_get_used(void);

/**
 *	Get the memory usage in system
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
mem_get_usage(void);


#endif /* end of FZ_MEM_UTIL  */


