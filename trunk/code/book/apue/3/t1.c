/*
 * write by jbug
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int _dup2(int fd1, int fd2)
{
    int*	fdopen;
    int		fd;
    int		cnt;
    int		i;
    
    if (fd2 < 0)
	return -1;

    fdopen = malloc(sizeof(int) * fd2);
    if (memset(fdopen, -1, fd2 * sizeof(int)) == NULL)
	return -1;
    
    for (i = 0; i < fd2; i++)
	printf("fdopen[%d] is %d\n", i, fdopen[i]);
    
    if (close(fd2) == -1)
	return -1;
    
    while ( (fd = dup(fd1)) != -1) {
	if (fd >= fd2)
	    break;
	fdopen[cnt++] = fd;
    }
    
    for (i = 0; i < fd2; i++)
	printf("fdopen[%d] is %d\n", i, fdopen[i]);
    
    for (cnt = 0; cnt < fd2; cnt++)
	if (fdopen[cnt] >= 0)
	    close(fdopen[cnt]);

    if (fd == fd2)
	return fd;
    else
	return -1;
}


int main(int argc, char** argv)
{
    int		val;

    if (argc != 3) {
	printf("usage: %s filedes1 filedes2\n", argv[0]);
	exit(1);
    }

    if ( (val = _dup2(atoi(argv[1]), atoi(argv[2]))) == -1) {
	printf("error: _dup2");
	exit(1);
    }

    printf("dupped filedes is %d\n", val);
    exit(0);
}
   
