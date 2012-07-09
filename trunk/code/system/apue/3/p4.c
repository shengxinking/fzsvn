/*
 * write by jbug
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    int		flag;
    int		accmode;

    if (argc != 2) {
	printf("usage: %s <file descriptor>\n", argv[0]);
	exit(1);
    }

    if ( (flag = fcntl(atoi(argv[1]), F_GETFL, 0)) == -1) {
	printf("error: fcntl for fd %d\n", atoi(argv[1]));
	perror("     : ");
	exit(1);
    }

    accmode = flag & O_ACCMODE;
    if (accmode == O_RDONLY) printf("read only");
    else if (accmode == O_WRONLY) printf("write only");
    else if (accmode == O_RDWR) printf("read write");
    else printf("error access mode\n");

    if (flag & O_APPEND) printf(", append");
    if (flag & O_NONBLOCK) printf(", nonblocking");
    if (flag & O_SYNC) printf(", synchronous writes");
    putchar('\n');

    exit(0);
}
