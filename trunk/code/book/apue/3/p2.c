/*
 * write by jbug
 */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    int		    fd;
    char	    buf1[] = "hello, I'm Stue.";
    char	    buf2[] = "hello, I'm jbug.";
    int		    len1, len2;
    
    if (argc != 2) {
	printf("usage: %s <filename>\n", argv[0]);
	exit(1);
    }

    if ( (fd = open(argv[1], 
		    O_RDWR | O_CREAT, 
		    S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP)) == -1) {
	printf("error: creat file: %s", argv[1]);
	perror("reason: ");
	exit(1);
    }

    len1 = strlen(buf1);
    if (write(fd, buf1, len1) != len1) {
	printf("error: write to file %s use data: %s\n", argv[1], buf1);
	perror("     :");
	exit(1);
    }

    if (lseek(fd, 16, SEEK_END) == -1) {
	printf("error: seek after file end 10\n");
	perror("     :");
	exit(1);
    }

    len2 = strlen(buf2);
    if (write(fd, buf2, len2) != len2) {
	printf("error: write to file %s use data: %s\n", argv[1], buf2);
	perror("     : ");
	exit(1);
    }

    close(fd);

    exit(0);
}
