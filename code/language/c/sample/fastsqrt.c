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

static float fast_sqrt(float f)
{
    float xhalf = 0.5f * f;
    int i = *(int *)&f;

    i = 0x5f3759df - (i >> 1);
    f = *(float *)&i;
    f = f * (1.5f - xhalf * f * f);

    return (1.0f / f);
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
	printf("%f quadratic root is %f\n", ret, fast_sqrt(ret));
    else {
	__usage();
	return -1;
    }

    return 0;
}
