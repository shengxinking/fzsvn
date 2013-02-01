/*
 *	kth:	implement find the k-th element in array
 *
 *	author:	forrest.zhang
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "kth.h"

int kth_array(void *array, size_t n, size_t size, int k, kthcmp cmp, void *kth)
{
	int i;

	void *karray = NULL;

	/* verify parameter */
	if (! array || !cmp || !kth || size < 0 || n < 0 || k < 0 || k > n)
		return -1;

	/* construct a k-elements array*/ 
	karray = malloc(k * size);
	if (!karray)
		return -1;
	memcpy(karray, array, k * size);
	qsort(karray, k, size, cmp);	

	for (i = k; i < n; i++) {
		if (cmp(array + i * size, karray + (k - 1) * size) < 0) {
			memcpy(karray + (k - 1) * size, array + i * size, size);
			qsort(karray, k, size, cmp);
		}
	}

	memcpy(kth, karray + (k - 1) * size, size);
	free(karray);

	return 0;
}

int kth_sort(void *array, size_t n, size_t size, int k, kthcmp cmp, void *kth)
{
	void *karray = NULL;

	/* verify parameter */
	if (! array || !cmp || !kth || size < 0 || n < 0 || k < 0 || k > n)
		return -1;

	/* construct a k-elements array*/ 
	karray = malloc(n * size);
	if (!karray)
		return -1;
	memcpy(karray, array, n * size);
	qsort(karray, n, size, cmp);

	memcpy(kth, karray + (k - 1) * size, size);

	free(karray);

	return 0;
}
