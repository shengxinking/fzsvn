/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int main(void)
{
    struct termios		term;
    long			vdisable;

    if (isatty(STDIN_FILENO) == 0) {
	printf("standard input is not a terminal device\n");
	exit(1);
    }

    if ( (vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE)) < 0) {
	printf("fpathconf error or _POSIX_VDISABLE not in effect\n");
	exit(1);
    }

    if (tcgetattr(STDIN_FILENO, &term) < 0) {
	printf("get attribute of stdin error\n");
	perror(NULL);
	exit(1);
    }

    term.c_cc[VINTR] = vdisable;
    term.c_cc[VEOF] = 2;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0) {
	printf("set attribute of stdin error\n");
	perror(NULL);
	exit(1);
    }

    exit(0);
}

