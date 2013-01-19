/*
 *	maxsubseqsum.h:		header file of max subsequence sum problem solve algthrithm.
 *
 *	author:			forrest.zhang
 */

#ifndef __MAXSUBSEQSUM_H
#define __MAXSUBSEQSUM_H

#include <sys/types.h>

typedef int (*maxsubseqsum_add)(void *sum, const void *a, const void *b);
typedef int (*maxsubseqsum_cmp)(void *a, void *b);

int maxsubseqsum_n3(const void *arr, size_t objsize, size_t nobjs, 
		    maxsubseqsum_cmp cmp, maxsubseqsum_add add, void *ret);

int maxsubseqsum_n2(const void *arr, size_t objsize, size_t nobjs, 
		    maxsubseqsum_cmp cmp, maxsubseqsum_add add, void *ret);

int maxsubseqsum_div(const void *arr, size_t objsize, size_t nobjs, 
		    maxsubseqsum_cmp cmp, maxsubseqsum_add add, void *ret);

int maxsubseqsum_n(const void *arr, size_t objsize, size_t nobjs, 
		    maxsubseqsum_cmp cmp, maxsubseqsum_add add, void *ret);

#endif	/* end of __MAXSUBSEQSUM_H */

