/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "nb_splice.h"

int 
nb_splice_init(void)
{
	int fd;

	fd = open(NB_SPLICE_DEV, O_RDWR);
	return fd;
}

int 
nb_splice(int nbfd, int infd, int outfd, size_t max, u_int32_t flags)
{
	int ret;
	nb_splice_t ns;

	if (nbfd < 0 || infd < 0 || outfd < 0 || max < 1)
		return -1;

	ns.infd = infd;
	ns.outfd = outfd;
	ns.max = max;
	ns.flags = flags;
	ns.len = 0;
	ns.is_open = 0;

	ret = ioctl(nbfd, IOCTL_DO_SPLICE, &ns);
	if (ret < 0)
		return -1;
	
	ret = ns.len;

	/* closed */
	if (ns.len == 0 && ns.is_open == 0)
		return -1;

	return ret;
}


