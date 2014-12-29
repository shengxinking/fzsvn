/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    pid_t		cpid;

    if ( (cpid = fork()) < 0)
	_exit(127);
    else if (cpid == 0) {
	sleep(1);
	if (setpgid(0, 0) < 0) {
	    printf("child setpgid error\n");
	    perror(NULL);
	    exit(1);
	}
	execl("group", "group", (char*)0);
	exit(0);
    }
    
    printf("child: pid = %d, gid = %d\n", cpid, getpgid(cpid));

    if (setpgid(0, cpid) < 0) {
	printf("parent setpgid error\n");
	perror(NULL);
	exit(1);
    }
    sleep(1);

//    printf("child: pid = %d, gid = %d\n", cpid, getpgid(cpid));

    
/*    if (setpgid(0, 0) < 0) {
	printf("parent setpgid error\n");
	perror(NULL);
	exit(1);
    }*/
    printf("parent: pid = %d, gid = %d\n", getpid(), getpgrp());

    exit(0);
} 
