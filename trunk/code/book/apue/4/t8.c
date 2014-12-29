/*
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
    int		    fd;

    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    if ( (fd = open(argv[1], O_RDONLY)) < 0) {
	printf("can't open file %s\n", argv[1]);
	perror("");
	exit(1);
    }

    if (unlink(argv[1]) < 0) {
	printf("can't unlink file %s\n", argv[1]);
	perror("");
	exit(1);
    }

    sleep(10);

    close(fd);

    exit(0);
}
