/*
 *  test read program for kernel module
 *
 *  write by Forrest.zhang
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#define BUF_SIZ            4
static char        buf[BUF_SIZ] = {0};

int main(int argc, char** argv)
{
    int         fd;

    if (argc != 2) {
	printf("usage: %s <file name>\n", argv[0]);
	return -1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
	printf("open file %s error: %s", argv[1], strerror(errno));
	return errno;
    }

//    while(read(fd, buf, BUF_SIZ - 1))
//	printf("%s", buf);

    sleep(30);

    close(fd);
    
    return 0;
}



