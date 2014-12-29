/*
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static void output(const char* str);

int main(void)
{
    pid_t	    cpid;

    if ( (cpid = fork()) < 0)
	_exit(1);
    else if (cpid == 0)
	output("output from child\n");
    else
	output("output from parent\n");

    exit(0);
}

static void output(const char* str)
{
    char	*ptr;
    int		c;

//    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
//	printf("setbuf error\n");
//	perror(NULL);
//    }
    for(ptr = str; (c = *ptr) != 0; ptr++) {
	sleep(1);
	putc(c, stdout);
    }
}



