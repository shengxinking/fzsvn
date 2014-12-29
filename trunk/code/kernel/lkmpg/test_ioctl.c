/*
 *
 *
 */

#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#define IOCTL_SET_MSG                0xeeff01
#define IOCTL_GET_MSG                0xeeff02
#define IOCTL_CLR_MSG                0xeeff03

int main(int argc, char** argv)
{
    int       fd;
    char      buf[513] = {0};

    if (argc != 2) {
	printf("usage: %s <file name>\n", argv[0]);
	return -1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
	printf("user: open file %s error: %s\n", argv[1], strerror(errno));
	return -1;
    }

    strcpy(buf, "Hi, this is Forrest.zhang!");
    if (ioctl(fd, IOCTL_SET_MSG, buf) < 0) {
	printf("user: ioctl set file %s error: %s\n", argv[1], strerror(errno));
	return -1;
    }

    memset(buf, 0, 513);
    if (ioctl(fd, IOCTL_GET_MSG, buf) < 0) {
	printf("user: ioctl get file %s error: %s\n", argv[1], strerror(errno));
	return -1;
    }
    printf("user: ioctl get: %s\n", buf);

    memset(buf, 0, 513);
    if (ioctl(fd, IOCTL_CLR_MSG, buf) < 0) {
	printf("user: ioctl clear file %s error: %s\n", argv[1], strerror(errno));
	return -1;
    }
    printf("user: ioctl clear: %s\n", buf);

    return 0;
}
