/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void func1(void);
static void func2(void);

int main(void)
{
    if (atexit(func1) != 0) {
	printf("can't register my_exit\n");
	perror(NULL);
	exit(1);
    }

    if (atexit(func2) != 0) {
	printf("can't register my_exit\n");
	perror(NULL);
	exit(1);
    }

    printf("main is done\n");
    exit(0);
}

static void func1(void)
{
    printf("exit handler func1\n");
}

static void func2(void)
{
    printf("exit handler func2\n");
}

