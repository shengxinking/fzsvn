/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define	DIR_MODE    (S_IRWXU | S_IRWXG | S_IRWXO)

int main(int argc, char** argv)
{
    char	    buf[4097];
    int		    i;
    
    if (argc != 2) {
	printf("usage: %s <pathname>\n", argv[0]);
	exit(1);
    }

    i = 0;
    while (i < 100) {
	if (mkdir(argv[1], DIR_MODE) < 0) {
	    printf("can't creat directory\n");
	    perror("");

	    if (getcwd(buf, 4096) == NULL) {
		printf("can't get current work directory\n");
		perror("");
	    }
	    else
		printf("current directory is: %s\n", buf);
	    exit(1);
	}
	else {
	    if (chdir(argv[1]) < 0) {
		printf("can't changer directory to t\n");
		perror("");
		exit(0);
	    }
	    
	    if (getcwd(buf, 4096) == NULL) {
		printf("can't get current work directory\n");
		perror("");
	    }
	    else
		printf("current directory is: %s\n", buf);
	    
	    ++i;
	}
    }

    exit(0);
}

