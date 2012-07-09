/*
 * @file	netfilter.c
 * @brief	some functions using in netfilter mode
 *
 * @author	Forrest.zhang
 */

#include <errno.h>
#include <string.h>

#include "debug.h"
#include "netfilter.h"

int nf_origin_dst(int fd, struct sockaddr_in *addr)
{
	socklen_t len;

	if (fd < 0)
		return -1;

	len = sizeof(struct sockaddr_in);
	if (getsockopt(fd, SOL_IP, SO_ORIGINAL_DST, addr, &len) != 0) {
		ERR("getsockopt(%d): %s\n", fd, strerror(errno));
		return -1;
	}

	return 0;
}






