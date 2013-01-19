/*
 * 
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

int main(int argc, char** argv)
{
    int			fd;

    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[1]);
	exit(1);
    }

    if (unlink(argv[1]) < 0) {
	printf("can't unlink file %s\n", argv[1]);
	perror("");
    }

    if ( (fd = creat(argv[1], FILE_MODE)) < 0) {
	printf("can't creat file %s\n", argv[1]);
	perror("");
	exit(1);
    }

    close(fd);
    exit(0);
}
