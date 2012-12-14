/*
 *	power.c:	using iterator implement X^Y
 *
 *	author:		forrest.zhang
 */

#include "power.h"

double power(double x, unsigned int y)
{
	if (y == 0)
		return 1;

	if (y == 1)
		return x;

	if (y % 2 == 0)
		return power(x * x, y / 2);
	else 
		return power(x * x, y / 2) * x;
}

