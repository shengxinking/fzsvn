/*
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
    int			fd;

    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    if (access(argv[1], R_OK) < 0) {
	printf("access read error\n");
	perror("reason");
    }
    else
	printf("access read OK\n");

    if ( (fd = open(argv[1], O_RDONLY | O_APPEND)) == -1) {
	printf("open read error\n");
	perror("reason");
    }
    else
	printf("open read OK\n");

    close(fd);

    exit(0);
}
