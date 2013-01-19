/*
 * write by jbug
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define BUFFSIZE    32768

int main(void)
{
    int		    n;
    char	    buf[BUFFSIZE];

    while ( (n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0) {
	if (write(STDOUT_FILENO, buf, n) != n) {
	    printf("error: write to stdout error\n");
	    perror("     : ");
	    exit(1);
	}
    }

    if (n < 0) {
	printf("error: read from stdin\n");
	perror("     : ");
	exit(1);
    }

    exit(0);
}
