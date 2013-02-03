/*
 *	gcd.c:		Euclid gcd algorithm implement
 *
 *	author:		forrest.zhang
 */

#include "gcd.h"

unsigned long 
gcd(unsigned long a, unsigned long b)
{
	unsigned long r;

	while (b > 0) {
		r = a % b;
		a = b;
		b = r;
	}

	return a;
}

