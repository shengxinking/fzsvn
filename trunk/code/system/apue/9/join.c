/*
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    if (argc != 2) {
	printf("usage: %s <#pgid>\n", argv[1]);
	exit(1);
    }
    
    printf("pid = %d, gid = %d\n", getpid(), getpgid(0));
    
    if (setpgid(0, atoi(argv[1])) < 0) {
	printf("setpgid error\n");
	perror(NULL);
	exit(1);
    }

    printf("pid = %d, gid = %d\n", getpid(), getpgid(0));
    
    exit(0);
}
