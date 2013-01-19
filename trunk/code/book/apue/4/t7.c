/*
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    int		    fd1, fd2;
    struct stat	    buf;
    int		    c, n;
    
    if (argc != 3) {
	printf("usage: %s file1 file2\n", argv[0]);
	exit(1);
    }

    if ( (fd1 = open(argv[1], O_RDONLY)) < 0) {
	printf("can't open file %s\n", argv[1]);
	perror("");
	exit(1);
    }

    if (fstat(fd1, &buf) < 0) {
	printf("can't get stat of file %s\n", argv[1]);
	perror("");
	exit(1);
    }

    if (access(argv[2], F_OK) < 0) {
	if ( (fd2 = creat(argv[2], buf.st_mode)) < 0) {
	    printf("can't creat file %s\n", argv[2]);
	    perror("");
	    exit(1);
	}
    }
    else {
	printf("file %s exist\n", argv[2]);
	exit(1);
    }

    while ( (n =read(fd1, &c, 1)) > 0) {
	if (c == 0)
	    lseek(fd2, 1, SEEK_CUR);
	else
	    write(fd2, &c, 1);
    }

    close(fd1);
    close(fd2);

    exit(0);
}
