/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stropts.h>
#include <unistd.h>

#define BUFFSIZE	1024

int main(void)
{
    int		    n, flag;
    struct strbuf   ctl, dat;
    char	    ctlbuf[BUFFSIZE], datbuf[BUFFSIZE];

    ctl.buf = ctlbuf;
    ctl.maxlen = BUFFSIZE;
    dat.buf = datbuf;
    dat.maxlen = BUFFSIZE;

    while (1) {
	flag = 0;
	if ( (n = getmsg(STDIN_FILENO, &ctl, &dat, &flag)) < 0) {
	    printf("getmsg error\n");
	    perror(NULL);
	    exit(1);
	}
	fprintf(stderr, "flag = %d, ctl.len = %d, dat.len = %d\n", 
		flag, ctl.len, dat.len);

	if (dat.len == 0)
	    exit(0);
	else if (dat.len > 0)
	    if (write(STDOUT_FILENO, dat.buf, dat.len) != dat.len) {
		perror(NULL);
		exit(1);
	    }
    }
}
