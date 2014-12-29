/*
 *  open a char device and sleep a moment, then close it
 *
 *  write by Forrest.zhang
 */

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

static void usage(void)
{
    printf("test_open <device file>\n");
}

int main(int argc, char** argv)
{
    int           fd;

    if (argc != 2) {
	usage();
	return -1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
	printf("open %s failed: %s", argv[1], strerror(errno));
	return -1;
    }

    sleep(60);

    close(fd);

    return 0;
}
