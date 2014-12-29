/*
 * write by jbug
 */

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    int		    fd;
    char	    buf[11];
    int		    n;
    
    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    if ( (fd = open(argv[1], O_RDWR)) == -1) {
	printf("error: open file %s\n", argv[1]);
	perror("     : ");
	exit(1);
    }

    if ( (n = read(fd, buf, 10)) < 0) {
	printf("error: read from file %s\n", argv[1]);
	perror("     : ");
	exit(1);
    }
    buf[n] = 0;
    
    printf("read 10 bytes from file %s: %s\n", argv[1], buf);
    
    if (ftruncate(fd, 100) < 0) {
	printf("error: ftruncate file\n");
	perror(NULL);
	exit(1);
    }

//    fsync(fd);
    
    if (lseek(fd, 10, SEEK_SET) < 0) {
	printf("error: lseek file error\n");
	perror(NULL);
	exit(1);
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
	printf("error: lseek file error\n");
	perror(NULL);
	exit(1);
    }
    
    if ( (n = read(fd, buf, 10)) < 0) {
	printf("error: read from file %s\n", argv[1]);
	perror("     : ");
	exit(1);
    }
    buf[n] = 0;
    
    printf("read 10 bytes from file %s: %s\n", argv[1], buf);
    
    exit(0);
}
