/*
 *	binarysearch.c:		binary search implement. I hypothesis the data is ascend sorted
 *
 * 	author:			forrest.zhang
 */

#include "binarysearch.h"

/*
 *	binarysearch:	search @val in a ascend sorted array @arr.
 *	@arr		ascend array
 *	@objsize	element size
 *	@nobjs		number of elements
 *	@cmp		function pointer using compare two elements
 *	@val		the value which need search
 *
 * 	return a pointer to the element if OK, NULL on error.
 */
const void *binarysearch(const void *arr, size_t objsize, size_t nobjs, binarysearch_cmp cmp, const void *val)
{
	int low, mid, high;

	if (!arr || !val)
		return 0;

	if (objsize < 1 || nobjs < 1)
		return 0;

	low = 0; 
	high = nobjs - 1;
	while (low < high) {
		mid = (high + low) / 2;
		if (cmp(val, arr + mid * objsize) < 0)
			high = mid - 1;
		else if (cmp(val, arr + mid *objsize) > 0)
			low = mid + 1;
		else
			return (arr + mid * objsize);
	}

	return 0;
}



