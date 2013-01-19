/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int main(void)
{
    char	    buf[L_ctermid];
    int		    fd;
    ssize_t	    n;
    
    printf("terminal name is: %s\n", ctermid(buf));
    
    if ( (fd = open(buf, O_RDWR)) < 0) {
	printf("open %s error\n", buf);
	perror(NULL);
	exit(1);
    }

    while ( (n = read(fd, buf, 5)) > 0)
	write(fd, buf, n);

    if (n < 0) {
	printf("read error\n");
	perror(NULL);
	exit(1);
    }
    
    exit(0);
}

