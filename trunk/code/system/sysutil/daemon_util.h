/**
 *	@file	daemon.h
 *
 *	@brief	The header file
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FW_DAEMON_H
#define FW_DAEMON_H

#define DN_RUN_PATH		"/var/run"

#define DN_MSG_OK		1
#define DN_MSG_CMD		2

typedef struct dn_msg {
	u_int32_t	pid;	/* the send program pid */
	u_int32_t	type;	/* the message type */
	u_int32_t	len;	/* the message len */
	char		msg[0];	/* the message */
} dn_msg_t;


/**
 *	Check the daemon is running or not
 *
 *	Return 0 if not exist, 1 if is running.
 */
extern int 
dn_is_running(const char *progname);


/**
 *	Init dameon @progname when it's running, it'll write a pid 
 *	file into @DN_PIDFILE_PATH/@progname.pid, and register some
 *	signal function, this function is used by @init program to 
 *	control this daemon(stop, update, etc...).
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dn_init(const char *progname);


/**
 *	Create a unix socket for daemon @progname.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dn_unix(const char *progname);


/**
 *	Send a message @msg to daemon.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dn_send_msg(const char *progname, dn_msg_t *msg);


/**
 *	Recv a message from unix socket, the message store in buf, it'll
 *	convert dn_msg_t struct and return.
 *
 *	Return ptr if success, NULL on error.
 */
extern dn_msg_t * 
dn_recv_msg(char *buf, int len);



/**
 *	Free the resource of daemon
 *
 *	No return.
 */
extern int 
dn_free(const char *progname);

#endif /* end of FW_DAEMON_H */

