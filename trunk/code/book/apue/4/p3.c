/*
 *  write by jbug
 */ 

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    int			fd;
    mode_t		oldmode;
    
    if (argc != 3) {
	printf("usage: %s <file1> <file2>\n", argv[0]);
	exit(1);
    }

    oldmode = umask(0);
    if (open(argv[1], O_RDONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
	printf("create file %s error: ", argv[1]);
	perror("");
	exit(1);
    }

    umask(0644);
    if (open(argv[2], O_RDONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
	printf("create file %s error: ", argv[2]);
	perror("");
	exit(1);
    }

    exit(0);
}
