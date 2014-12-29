/*
 *
 */

#include <stdio.h>
#include <stdlib.h>

static void pr_stdio(const char* name, FILE* fp);

static void _setbuf(FILE* fp, char* buf)
{
    if(buf == NULL)
	setvbuf(fp, NULL, _IONBF, 1);
    else {
	setvbuf(fp, buf, _IOFBF, BUFSIZ);
    }
}

int main(void)
{
    char		buf[5], fbuf[5];

    if (setvbuf(stdin, fbuf, _IOFBF, 5)) {
	printf("setvbuf to stdin error\n");
	perror("");
	exit(1);
    }

    if (setvbuf(stderr, buf, _IOFBF, 5)) {
	printf("setvbuf to stdout error\n");
	perror("");
	exit(1);
    }

    _setbuf(stdout, NULL);

    fputs("hello stdout\n", stdout);
    fgetc(stdin);
    fputs("hello stderr\n", stderr);

    pr_stdio("stdin", stdin);
    pr_stdio("stdout", stdout);
    pr_stdio("stderr", stderr);

    return 0;
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

