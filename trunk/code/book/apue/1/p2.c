/*
 *  write by jbug<thangguo@yahoo.com.cn>\
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFSIZE    100

int main(int argc, char** argv)
{
    int		    n;
    char	    buf[BUFFSIZE];

    while ( (n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0)
	if (write(STDOUT_FILENO, buf, n) != n) {
	    printf("error: write to stdout: %s\n", buf);
	    perror("reason: ");
	    exit(1);
	}

    if (n < 0) {
	printf("error: read from stdin\n");
	perror("reason: ");
	exit(1);
    }

    exit(0);
}
