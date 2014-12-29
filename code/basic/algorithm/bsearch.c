/*
 *	bsearch.c:	binary search implement. I hypothesis the 
 *			data is ascend sorted
 *
 * 	author:		forrest.zhang
 */

#include "algorithm.h"


const void * 
bin_search(const void *array, size_t objsize, size_t nobj, 
	   bin_search_cmp_func cmp, const void *val)
{
	int low, mid, high;
	if (!array || !val)
		return 0;

	if (objsize < 1 || nobj < 1)
		return 0;

	low = 0; 
	high = nobj - 1;
	while (low < high) {
		mid = (high + low) / 2;
		if (cmp(val, array + mid * objsize) < 0)
			high = mid - 1;
		else if (cmp(val, array + mid *objsize) > 0)
			low = mid + 1;
		else
			return (array + mid * objsize);
	}

	return 0;
}



