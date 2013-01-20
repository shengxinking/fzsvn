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

#include "sysutil.h"

#define	PID_FILE_FMT	"/var/log/%s.pid"

int 
pid_file_exist(const char *pname)
{
	char pidfile[PID_FILE_NAMELEN];

	if (!pname)
		return 0;

	snprintf(pidfile, PID_FILE_NAMELEN, PID_FILE_FMT, pname);

	if (access(pidfile, F_OK) == 0)
		return 1;

	return 0;
}


int 
pid_file_new(const char *pname, pid_t pid)
{
	char pidfile[PID_FILE_NAMELEN];
	char pid_str[10];
	int fd;
	size_t len;
	ssize_t n;

	if (!pname || pid < 1)
		return 0;

	snprintf(pidfile, PID_FILE_NAMELEN, PID_FILE_FMT, pname);

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
pid_file_del(const char *pname)
{
	char pidfile[PID_FILE_NAMELEN];

	if (!pname)
		return 0;

	snprintf(pidfile, PID_FILE_NAMELEN, PID_FILE_FMT, pname);

	if (access(pidfile, F_OK))
		return -1;

	unlink(pidfile);

	return 0;
}


pid_t 
pid_file_read(const char *pname)
{
	char pidfile[PID_FILE_NAMELEN];
	char pid_str[10];
	int fd;
	size_t len;
	ssize_t n;
	int i = 0;

	if (!pname)
		return 0;

	snprintf(pidfile, PID_FILE_NAMELEN, PID_FILE_FMT, pname);

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



