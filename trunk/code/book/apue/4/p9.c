/*
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    char	    buf[1024];
    
    if (getcwd(buf, 1024) == NULL) {
	printf("can't get current work directory\n");
	perror("");
	exit(1);
    }

    printf("current work directory is %s\n", buf);

    exit(0);
}
