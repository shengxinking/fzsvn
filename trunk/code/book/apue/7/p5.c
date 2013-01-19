/*
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

static jmp_buf	    jmpbuf;

static void func1(int, int, int);
static void func2(void);

int main(void)
{
    int		    cnt;
    register int    val;
    volatile int    sum;

    cnt = 2; val = 3; sum = 4;
    if (setjmp(jmpbuf) != 0) {
	printf("after longjmp: cnt = %d, val = %d, sum = %d\n", cnt, val, sum);
	exit(0);
    }

    cnt = -2; val = -3; sum = -4;

    func1(cnt, val, sum);
    exit(0);
}

static void func1(int i, int j, int k)
{
    printf("in func1: cnt = %d, val = %d, sum = %d\n", i, j, k);
    func2();
}

static void func2(void)
{
    longjmp(jmpbuf, 1);
}
