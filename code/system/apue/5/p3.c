/*
 *
 */

#include <stdio.h>
#include <stdlib.h>

static void pr_stdio(const char*, FILE*);

int main(void)
{
    FILE*	    fp;

    fputs("enter any character\n", stdout);
    if (getchar() == EOF) {
	printf("getchar error\n");
	perror("");
    }

    fputs("one line to standard error\n", stderr);

    pr_stdio("stdin", stdin);
    pr_stdio("stdout", stdout);
    pr_stdio("stderr", stderr);

    if ( (fp = fopen("/etc/passwd", "r")) == NULL) {
	printf("can't open /etc/passwd\n");
	perror("");
    }

    pr_stdio("/etc/passwd", fp);

    exit(0);
}

void pr_stdio(const char* name, FILE* fp)
{
    printf("stream = %s, ", name);
    if (fp->_flags & _IONBF)
	printf("unbuffered");
    else if (fp->_flags & _IOLBF)
	printf("line buffered");
    else
	printf("fully buffered");
    printf(", buffer size = %d\n", (fp->_IO_buf_end - fp->_IO_buf_base));
}
