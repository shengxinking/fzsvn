/**
 *	@file	
 *
 *	@brief
 *	
 *	@author	Forrest.zhang	
 *
 *	@date
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>

static inline int 
inline_test(int i) 
{
	return ++i;
}


int 
main(void)
{
	int i = 0;

	i = inline_test(i);

	return i;
}


