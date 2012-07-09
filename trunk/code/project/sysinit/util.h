/**
 *	@file	util.h
 *
 *	@brief	The utils API for init.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#include <sys/types.h>

#ifndef FZ_UTIL_H
#define FZ_UTIL_H

/* the realtime signal for init */
#define SIG_RAID		35
#define SIG_FORMAT		36
#define SIG_UPDATE		37
#define SIG_UPGRADE		38
#define SIG_STOP_RESPAWN2	39
#define SIG_START_RESPAWN2	40
#define SIG_REBOOT		41
#define SIG_SHUTDOWN		42

/* the sig_info code for realtime signature */
#define SIG_FORMAT_LOG		1
#define SIG_FORMAT_HOME		2

#define SIG_UPDATE_CLI		1
#define SIG_UPDATE_SIGNATURE	2

#define SIG_RESPAWN2_NTP	1
#define SIG_RESPAWN2_SNMP	2
#define SIG_RESPAWN2_ALERT	3


/**
 *	Sleep @msec microsecond.
 *
 *	No return.
 */
extern void 
ut_mdelay(int msec);

/**
 *	Wait the process @pid exit.
 *
 *	Return the process return value.
 */
extern int 
ut_waitfor(pid_t pid);

/**
 *	Run the command and wait the return value.
 *
 *	Return the program return value.
 */
extern int
ut_runcmd(const char *cmd);


/**
 *	Set the console for init program
 *
 *	No return.
 */
extern int 
ut_set_console(const char *dev, int isctty);


#endif /* end of FZ_UTIL_H  */

