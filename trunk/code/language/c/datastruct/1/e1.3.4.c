/*
 *  the cube root algorithm
 *
 *  write by Forrest.zhang
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "test.h"

#define CR_PRECISE            0.0000001

double cube_root(double x)
{
	double z = x;
	double y;

	while ( (y = ( (z * z * z) - x) ) > CR_PRECISE || y < -(CR_PRECISE)) {
		printf("y = %f, z = %f\n", y, z);
		z = (2 * z + x / (z * z)) / 3;
	}

	return z;
}


int main(int argc, char **argv)
{
	double x;

	if (argc != 2)
		return -1;

	printf("argv[1] is %s\n", argv[1]);

	x = strtod(argv[1], 0);

	printf("%f cube root is %f\n", x, cube_root(x));

	return 0;
}
