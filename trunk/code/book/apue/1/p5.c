/*
 *
 */

#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define		    BUFFSIZE	    100

int main(void)
{
    char	    buf[BUFFSIZE];
    pid_t	    pid;
    int		    status;
    
    printf("%% ");
    while (fgets(buf, BUFFSIZE, stdin) != NULL) {
	buf[strlen(buf) - 1] = 0;

	if ( (pid = fork()) < 0) {
	    printf("error: can't fork\n");
	    perror("reason: ");
	    exit(1);
	}
	else if (pid == 0) {
	    execlp(buf, buf, (char*)0);
	    printf("error: can't execlp: %s\n", buf);
	    perror("reason: ");
	    exit(1);
	}
	
	if ( (pid = waitpid(pid, &status, 0)) < 0) {
	    printf("error: waitpid\n");
	    perror("reason: ");
	    exit(1);
	}
	printf("%%");
    }

    exit(0);
}
