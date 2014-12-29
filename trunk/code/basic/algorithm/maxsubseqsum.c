/*
 *	maxsubseqsum:	return the max subsequence sum of a array
 *
 * 	author:		forrest.zhang
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#include "maxsubseqsum.h"

/*	maxsubseqsum_n3:	using 3-level for loop do the work, O(N^3)
 *	@arr			data array
 *	@objsize		every element size
 * 	@nobjs			number of element in array @arr
 *	@cmp			the function compare two element
 *	@ret			the max subsequence sum stored here for return
 *
 * 	return 0 if OK, -1 on error.
 */
int maxsubseqsum_n3(const void *arr, size_t objsize, size_t nobjs, 
		    maxsubseqsum_cmp cmp, maxsubseqsum_add add, void *ret)
{
	int i, j, k;
	void *sum = NULL;

	if (!arr || !cmp || !ret)
		return -1;

	if (objsize < 1 || nobjs < 1)
		return -1;

	sum = malloc(objsize);
	if (!sum)
		return 0;
	memset(sum, 0, nobjs);

	memcpy(ret, arr, objsize);
	for (i = 0; i < nobjs; i++) {
		for (j = i; j < nobjs; j++) {
			memset(sum, 0, objsize);
			for (k = i; k <= j; k++)
				add(sum, sum, arr + k * objsize);

			if (cmp(sum, ret) > 0)
				memcpy(ret, sum, objsize);
		}
	}

	return 0;
}

/*	maxsubseqsum_n2:	using two-level for loop do the work, O(N^2)
 *	@arr			data array
 *	@objsize		every element size
 * 	@nobjs			number of element in array @arr
 *	@cmp			the function compare two element
 *	@ret			the max subsequence sum stored here for return
 *	
 *	return 0 if OK, -1 on error
 */
int maxsubseqsum_n2(const void *arr, size_t objsize, size_t nobjs,
		    maxsubseqsum_cmp cmp, maxsubseqsum_add add, void *ret)
{
	int i, j;
	void *sum = NULL;

	if (!arr || !cmp || !ret)
		return -1;

	if (objsize < 1 || nobjs < 1)
		return -1;

	sum = malloc(objsize);
	if (!sum)
		return -1;
	memset(sum, 0, objsize);

	memcpy(ret, arr, objsize);
	for (i = 0; i < nobjs; i++) {
		memset(sum, 0, objsize);
		for (j = i; j < nobjs; j++) {
			add(sum, sum, arr + j * objsize);
			
			if (cmp(sum, ret) > 0)
				memcpy(ret, sum, objsize);
		}
	}

	return 0;
}

/*	maxsubseqsum_div:	using divide-and-conquer do the work, O(N^2)
 *	@arr			data array
 *	@objsize		every element size
 * 	@nobjs			number of element in array @arr
 *	@cmp			the function compare two element
 *	@ret			the max subsequence sum stored here for return
 *	
 *	return 0 if OK, -1 on error
 */
int maxsubseqsum_div(const void *arr, size_t objsize, size_t nobjs,
		   maxsubseqsum_cmp cmp, maxsubseqsum_add add, void *ret)
{
	int i;
	void *maxleftsum = NULL;
	void *maxrightsum = NULL;
	void *maxleftbordersum = NULL;
	void *maxrightbordersum = NULL;
	void *leftbordersum = NULL;
	void *rightbordersum = NULL;
	size_t left = 0, right = 0;

	if (!arr || !cmp || !ret)
		return -1;

	if (objsize < 1 || nobjs < 1)
		return -1;

	if (nobjs == 1) {
		memcpy(ret, arr, objsize);
		return 0;
	}
	
	maxleftsum = malloc(objsize * 6);
	if (!maxleftsum)
		return -1;
	memset(maxleftsum, 0, objsize * 6);
	maxrightsum = maxleftsum + objsize;
	maxleftbordersum = maxleftsum + 2 * objsize;
	maxrightbordersum = maxleftsum + 3 * objsize;
	leftbordersum = maxleftsum + 4 * objsize;
	rightbordersum = maxleftsum + 5 * objsize;

	memcpy(ret, arr, objsize);

	left = nobjs / 2;
	right = nobjs - left;
	maxsubseqsum_div(arr, objsize, left, cmp, add, maxleftsum);
	maxsubseqsum_div(arr + left * objsize, objsize, right, cmp, add, maxrightsum);

	for (i = (left - 1); i >= 0; i--) {
		add(leftbordersum, leftbordersum, arr + i * objsize);
		if (cmp(leftbordersum, maxleftbordersum))
			memcpy(maxleftbordersum, leftbordersum, objsize);
	}

	for (i = left; i < nobjs; i++) {
		add(rightbordersum, rightbordersum, arr + i * objsize);
		if (cmp(rightbordersum, maxrightbordersum) > 0)
			memcpy(maxrightbordersum, rightbordersum, objsize);
	}

	add(maxleftbordersum, maxleftbordersum, maxrightbordersum);

	if (cmp(maxleftsum, ret) > 0)
		memcpy(ret, maxleftsum, objsize);
	
	if (cmp(maxrightsum, ret) > 0)
		memcpy(ret, maxrightsum, objsize);

	if (cmp(maxleftbordersum, ret) > 0)
		memcpy(ret, maxleftbordersum, objsize);

	return 0;
}

int maxsubseqsum_n(const void *arr, size_t objsize, size_t nobjs,
		   maxsubseqsum_cmp cmp, maxsubseqsum_add add, void *ret)
{
	void *sum = NULL;
	int i;

	if (!arr || !cmp || !ret)
		return -1;

	if (objsize < 1 || nobjs < 1)
		return -1;

	sum = malloc(objsize);
	if (!sum)
		return -1;
	memset(sum, 0, objsize);

	memcpy(ret, arr, objsize);
	for (i = 0; i < objsize; i++) {
		add(sum, sum, arr + i * objsize);
		if (cmp(sum, ret) > 0)
			memcpy(ret, sum, objsize);
		else if (cmp(sum, NULL) < 0)
			memset(sum, 0, objsize);
	}

	return 0;
}

