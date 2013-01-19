/*
 * write by jbug
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char** argv)
{
    int		    fd;
    char	    buf[10];

    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    if ( (fd = open(argv[1], O_RDWR | O_APPEND)) == -1) {
	printf("error: open file %s\n", argv[1]);
	perror("     : ");
	exit(1);
    }

    if ( lseek(fd, 0, SEEK_SET) == -1) {
	printf("error: lseek file %s\n", argv[1]);
	perror("     : ");
	exit(1);
    }

    if (read(fd, buf, 10) < 0) {
	printf("error: read from file %s\n", argv[1]);
	perror("     : ");
	exit(1);
    }

    if (write(fd, "jbugjbug", 8) != 8) {
	printf("error: write to file %s\n", argv[1]);
	perror("     : ");
	exit(1);
    }
    
    close(fd);
    exit(0);
}
