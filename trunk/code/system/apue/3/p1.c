/*
 *  write by jbug
 */
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    if (lseek(STDIN_FILENO, 0, SEEK_CUR) == -1) {
	printf("stdin cann't seek\n");
	exit(0);
    }
    else
	printf("stdin can be seek\n");

    exit(0);
}
