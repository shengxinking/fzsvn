/*
 *  write by jbug
 */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif

int main(int argc, char** argv)
{
    int		    fd, fd1, fd2 = 1;
    char	    buf[] = "Hello, my name is Elen Stuarte.";
    off_t	    pos;
    ssize_t	    n;
    
    if (argc != 2) {
	printf("usage: %s <filename>", argv[0]);
	exit(0);
    }

    if ( (fd = open(argv[1], O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
	printf("error: open file: %s\n", argv[1]);
	perror("     : ");
//	exit(1);
    }
    printf("fd is %d\n", fd);

    if ( (fd1 = dup(fd)) == -1) {
	printf("error: dup\n");
	perror("     : ");
    }
    printf("duped fd is %d\n", fd1);

    if ( (fd2 = dup2(fd, 0)) == -1) {
	printf("error: dup2 to 1\n");
	perror("     : ");
    }
    printf("dup2ed fd is %d\n", fd2);

    if ( (fd1 = fcntl(fd, F_DUPFD, 10)) == -1) {
	printf("error: fcntl's dupfd function\n");
	perror("     : ");
    }
    printf("fcntl duped fd is %d\n", fd1);

    if ( (fd1 = fcntl(fd, F_DUPFD, 10)) == -1) {
	printf("error: fcntl's dupfd function\n");
	perror("     : ");
    }
    
    printf("fcntl duped fd is %d\n", fd1);
    close(0);
    if ( (fd2 = fcntl(fd, F_DUPFD, 0)) == -1) {
	printf("error: fcntl's dupfd function\n");
	perror("     : ");
    }
    printf("fcntl dup2ed fd is %d\n", fd2);

//    if (write(fd, buf, 30) != 30) {
//	printf("error: write to file %s using data: %s\n", argv[1], buf);
//	perror("     : ");
//	exit(1);
//    }

//    if ( (pos = lseek(fd, -230, SEEK_END)) == -1) {
//	pos = lseek(fd, 0, SEEK_CUR);
//	printf("pos is %ld\n", pos);
//	printf("error: seek file %s\n", argv[1]);
//	perror("     : ");
//	exit(1);
//    }

//    if ( (n = read(fd, buf, 30)) != 30) {
//	printf("read %d bytes\n", n);
//	printf("error: read date from file %s\n", argv[1]);
//	perror("     : ");
//	exit(1);
//    }
    
    close(fd);
    exit(0);
}
