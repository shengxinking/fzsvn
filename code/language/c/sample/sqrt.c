/*
 *  the quadratic root test program
 *
 *  write by Forrest.zhang in Fortinet Inc.
 */

#include <stdio.h>
#include <stdlib.h>

static void __usage(void)
{
    printf("sqrt <positive number>\n");
}

static const double PRECISE = 1.0e-8;

static double __sqrt(double x)
{
    double y;

    if (x <= 0)
	return 0;

    y = x / 2;
    while ( (y * y - x) > PRECISE ||
	    (x - y * y) > PRECISE) 
	y = (y + x / y) / 2.0;

    return y;
}

int main(int argc, char **argv)
{
    double    ret = 0;

    if (argc != 2) {
	__usage();
	return -1;
    }

    ret = atol(argv[1]);
    
    if (ret > 0)
	printf("%f quadratic root is %f\n", ret, __sqrt(ret));
    else {
	__usage();
	return -1;
    }

    return 0;
}
