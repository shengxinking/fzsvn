/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int	    glob = 6;
char		    buf[] = "a write to stdout\n";

int main(void)
{
    int		val;
    pid_t	cpid;

    val = 88;
    if (write(STDOUT_FILENO, buf, sizeof(buf) - 1) != (sizeof(buf) -1)) {
	printf("write to stdout error\n");
	perror(NULL);
	exit(1);
    }

    if ( (cpid = fork()) < 0) {
	printf("fork error\n");
	perror(NULL);
	exit(1);
    }
    else if (cpid == 0) {
	glob++;
	val++;
    }
//    else
//	sleep(2);

    printf("pid = %d, glob = %d, val = %d\n", getpid(), glob, val);

    exit(0);
}
