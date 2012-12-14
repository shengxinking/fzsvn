/*
 *	binarysearch.h:		the header file of binary search
 *
 *	author:			forrest.zhang
 */

#ifndef __BINARYSEARCH_H
#define __BINARYSEARCH_H

#include <sys/types.h>

typedef int (*binarysearch_cmp)(const void *a, const void *b);

extern const void *binarysearch(const void *arr, size_t objsize, size_t nobjs, binarysearch_cmp cmp, const void *val);

#endif /* end of __BINARYSEARCH_H */

