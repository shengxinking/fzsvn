/**
 *	@file	daemon.c
 *
 *	@brief	The damon APIs, it's using by all daemons.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2010-07-21
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX	108
#endif

#include "daemon.h"


static int _dn_unfd = -1;

/**
 *	write process id to a pid file %DN_PIDFILE_PATH/@progname.pid.
 *	
 *	return 0 if OK, -1 on error.
 */
int _dn_write_pidfile(const char *progname)
{
	int fd = -1;
	int pid = 0;
	char s_pid[20] = {0};
	char pidfile[128] = {0};
	
	snprintf(pidfile, 127, "%s/%s.pid", DN_RUN_PATH, progname);

	fd = open(pidfile, O_WRONLY | O_TRUNC | O_CREAT | O_SYNC, 0644);
	if (fd < 0) {
		return -1;
	}
	
	pid = getpid();
	memset(s_pid, 0, 20);
	snprintf(s_pid, 19, "%d", pid);
	if (write(fd, s_pid, strlen(s_pid)) != strlen(s_pid)) {
		close(fd);
		unlink(pidfile);
		return -1;
	}
	
	close(fd);
	
	return 0;
}


/**
 *	removed pid file %DN_PIDFILE_PATH/@progname.pid if exist.
 *
 *	return 0 if OK, -1 on error
 */
static int _dn_remove_pidfile(const char *progname)
{
	char pidfile[128];

	snprintf(pidfile, 127, "%s/%s.pid", DN_RUN_PATH, progname);

	if (access(pidfile, R_OK) == 0)
		unlink(pidfile);

	return 0;
}

/**
 *	Create a unix socket for daemon @progname.
 *
 *	Return the socket fd if success, -1 on error.
 */
int 
_dn_unix_socket(const char *progname)
{
	struct sockaddr_un un;
	int fd;

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("create AF_UNIX socket failed: %s\n", strerror(errno));
		return -1;
	}
	
	snprintf(un.sun_path, UNIX_PATH_MAX - 1, "%s/%s.sock",
		 DN_RUN_PATH, progname);
	un.sun_family = AF_UNIX;

	/* remove the old socket file */
	unlink(un.sun_path);
	
	/* bind */
	if (bind(fd, (struct sockaddr *)&un, sizeof(un))) {
		printf("bind AF_UNIX socket failed: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	_dn_unfd = fd;

	return 0;

}

/**
 *	Check the daemon is running or not.
 *
 *	Return 0 if not exist, 1 if exist.
 */
int
dn_is_running(const char *progname)
{
	int fd;
	char proc_entry[48] = {0};
	char s_pid[20];
	char pidfile[128] = {0};
	
	if (!progname)
		return 0;
	
	snprintf(pidfile, 127, "%s/%s.pid", DN_RUN_PATH, progname);

	if (access(pidfile, R_OK) == 0) {
		fd = open(pidfile, O_RDONLY);
		if (fd < 0) {
			unlink(pidfile);
			return 0;
		}

		memset(s_pid, 0, 20);
		read(fd, s_pid, sizeof(s_pid)-1);

		if (strlen(s_pid) < 1) {
			close(fd);
			unlink(pidfile);
			return 0;
		}

		snprintf(proc_entry, 47, "/proc/%s", s_pid);

		if (s_pid[0] != '\0' && access(proc_entry, R_OK | X_OK) == 0) {
			close(fd);
			return 1;
		}

		close(fd);
		unlink(pidfile);
	}

	return 0;
}


/**
 *	Init dameon @progname.
 *
 *	Return 0 if success, -1 on error.
 */
int 
dn_init(const char *progname)
{
	if (!progname)
		return -1;

	if (dn_is_running(progname))
		return -1;
	
	if (_dn_write_pidfile(progname))
		return -1;
	
	if (_dn_unix_socket(progname))
		return -1;

	return 0;
}


/**
 *	Free the resource of daemon
 *
 *	No return.
 */
extern int 
dn_free(const char *progname)
{
	if (!progname)
		return -1;

	if (_dn_remove_pidfile(progname))
		return -1;
	
	if (_dn_unfd > -1)
		close(_dn_unfd);

	return 0;
}


/**
 *	Send a message @msg to daemon.
 *
 *	Return 0 if success, -1 on error.
 */
int 
dn_send_msg(const char *progname, dn_msg_t *msg)
{
	struct sockaddr_un un;
	int fd;
	int n;
	int len;

	if (!progname || !msg)
		return -1;

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("create UNIX socket failed: %s\n", strerror(errno));
		return -1;
	}
		
	/* send message */
	snprintf(un.sun_path, UNIX_PATH_MAX - 1, "%s/%s.sock", 
		 DN_RUN_PATH, progname);
	un.sun_family = AF_UNIX;	
	len = msg->len + sizeof(dn_msg_t);
	n = sendto(fd, (const char *)msg, len, 0, 
		   (struct sockaddr *)&un, sizeof(un));
	if (n < len) {
		printf("sendto error: %s\n", strerror(errno));
		return -1;
	}

	close(fd);

	return 0;
}


/**
 *	Recv a message from unix socket, the message store in buf, it'll
 *	convert dn_msg_t struct and return.
 *
 *	Return ptr if success, NULL on error.
 */
dn_msg_t * 
dn_recv_msg(char *buf, int len)
{
	int n;
	dn_msg_t *msg;

	if (_dn_unfd < 0)
		return NULL;
	
	n = recvfrom(_dn_unfd, buf, len, MSG_DONTWAIT, NULL, 0);
	if (n < 0 || n < sizeof(dn_msg_t))
		return NULL;
	
	/* verify message length */
	msg = (dn_msg_t *)buf;
	if (msg->len != n + sizeof(dn_msg_t))
		return NULL;

	return msg;
}

