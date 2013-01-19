/*
 *  write by jbug<thangguo@yahoo.com.cn>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int		c;
    
    while ( (c = getc(stdin)) != EOF)
	if (putc(c, stdout) == EOF) {
	    printf("error: put %c to stdout\n", c);
	    perror("reason: ");
	    exit(1);
	}

    if (ferror(stdin)) {
	printf("error: get a character from  stdin\n");
	perror("reason: ");
	exit(1);
    }

    exit(0);
}
