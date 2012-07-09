/*
 * write by jbug
 */

#include <sys/types.h>
#include <fcntl.h>

int set_fl(int fd, int flag)
{
    int		val;

    if ( (val = fcntl(fd, F_GETFL, 0)) < 0)
	return -1;

    val |= flag;

    if (fcntl(fd, F_SETFL, val) < 0)
	return -1;

    return val;
}

int clr_fl(int fd, int flag)
{
    int		val;

    if ( (val = fcntl(fd, F_GETFL, 0)) < 0)
	return -1;

    val &= ~flag;

    if (fcntl(fd, F_SETFL, val) < 0)
	return -1;

    return val;
}
