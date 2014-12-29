/**
 *	@file	sem_metux.h
 *
 *	@brief	The metux lock using semaphore.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2012-07-23
 */

#ifndef FZ_SEM_METUX_H
#define FZ_SEM_METUX_H


/**
 *	Create a mutex lock using semphore, the lock key is %key.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sem_mutex_create(int key);

/**
 *	Destroy a mutex lock , the lock key is %key.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sem_mutex_destroy(int key);

/**
 *	Lock the mutex lock @key
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sem_mutex_lock(int key);

/**
 *	Unlock the mutex lock @key
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
sem_mutex_unlock(int key);

/**
 *	Print the semphore val and pid, the lock key is @key.
 *
 *	Return 1 if deadlocked, 0 if not, -1 on error.
 */
extern int 
sem_mutex_print(int key);


#endif /* end of FZ_SEM_METUX_H  */


