/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
    char	    buf[100];
    int		    fd;
    
    if (argc != 2) {
	printf("usage: %s <pathname>\n", argv[1]);
	exit(1);
    }

    if (getcwd(buf, 100) == NULL) {
	printf("can't get current work directory\n");
	perror("");
	exit(1);
    }

    printf("current work directory: %s\n", buf);

    if (chdir(argv[1]) < 0) {
	printf("can't changer to directory %s\n", argv[1]);
	perror("");
//	exit(1);
    }

    if (getcwd(buf, 100) == NULL) {
	printf("can't get current work directory\n");
	perror("");
	exit(1);
    }

    printf("current work directory: %s\n", buf);

    if (chdir("/tmp") < 0) {
	printf("can't changer to directory /tmp\n");
	perror("");
	exit(1);
    }

    if (getcwd(buf, 100) == NULL) {
	printf("can't get current work directory\n");
	perror("");
	exit(1);
    }

    printf("current work directory: %s\n", buf);
    
    if ( (fd = open(argv[1], O_RDONLY)) < 0) {
	printf("can't not open directory %s\n", argv[1]);
	perror("");
	exit(1);
    }

    if (fchdir(fd) < 0) {
	printf("can't changer current work directory to %s\n", argv[1]);
	perror("");
	exit(1);
    }

    if (getcwd(buf, 100) == NULL) {
	printf("can't get current work directory\n");
	perror("");
	exit(1);
    }

    printf("current work directory: %s\n", buf);
 
    close(fd);
    exit(0);
}
