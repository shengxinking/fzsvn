/**
 *	@file	pidfile_util.c
 *
 *	@brief	the pidfile functions.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2012-08-03
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>

#include "pidfile_util.h"

#define	PIDFILE_FMT	"/var/log/%s.pid"

int 
pidfile_is_exist(const char *pname)
{
	char pidfile[PIDFILE_LEN];

	if (!pname)
		return 0;

	snprintf(pidfile, PIDFILE_LEN, PIDFILE_FMT, pname);

	if (access(pidfile, F_OK) == 0)
		return 1;

	return 0;
}


int 
pidfile_new(const char *pname, pid_t pid)
{
	char pidfile[PIDFILE_LEN];
	char pid_str[10];
	int fd;
	size_t len;
	ssize_t n;

	if (!pname || pid < 1)
		return 0;

	snprintf(pidfile, PIDFILE_LEN, PIDFILE_FMT, pname);

	fd = open(pidfile, O_CREAT | O_WRONLY | O_TRUNC, 0600);
	if (fd < 0)
		return -1;

	snprintf(pid_str, sizeof(pid_str), "%d", pid);

	len = strlen(pid_str);
	n = write(fd, pid_str, len);
	if (n < 0 || n != len) {
		close(fd);
		unlink(pidfile);
		return -1;
	}

	close(fd);
	return 0;
}


int 
pidfile_del(const char *pname)
{
	char pidfile[PIDFILE_LEN];

	if (!pname)
		return 0;

	snprintf(pidfile, PIDFILE_LEN, PIDFILE_FMT, pname);

	if (access(pidfile, F_OK))
		return -1;

	unlink(pidfile);

	return 0;
}


pid_t 
pidfile_get_pid(const char *pname)
{
	char pidfile[PIDFILE_LEN];
	char pid_str[10];
	int fd;
	size_t len;
	ssize_t n;
	int i = 0;

	if (!pname)
		return 0;

	snprintf(pidfile, PIDFILE_LEN, PIDFILE_FMT, pname);

	fd = open(pidfile, O_RDONLY);
	if (fd < 0)
		return -1;

	len = sizeof(pid_str);
	n = read(fd, pid_str, len);
	close(fd);

	if (n < 1 || n > (len - 1)) {
		return -1;
	}

	for (i = 0; i < n; i++) {
		if (!isdigit(pid_str[i]))
			return -1;
	}

	return atoi(pid_str);
}



