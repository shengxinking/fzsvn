/**
 *
 *
 *
 *
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include "sysutil.h"

int 
com_open(const char *dev, sighandler_t sig_io)
{
	int fd;

	if (!dev) {
		printf("Invalid argument\n");
		return -1;
	}

	signal(SIGIO, sig_io);

	fd = open(dev, O_RDONLY|O_NONBLOCK|O_ASYNC);
	if (fd < 0) {
		printf("open %s failed\n", dev);
		return -1;
	}

	return fd;
}

int 
com_close(int fd)
{
	if (fd < 0)
		return -1;

	signal(SIGIO, SIG_DFL);
	close(fd);

	return 0;
}



