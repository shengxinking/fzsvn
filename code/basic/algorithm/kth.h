/*
 *	kth:	find the k-th element in a array
 *
 * 	author:	forrest.zhang
 */

#ifndef _KTH_H
#define _KTH_H

#include <sys/types.h>

typedef int (*kthcmp)(const void *i, const void *j);

extern int kth_array(void *arr, size_t n, size_t size, int k, kthcmp cmp, void *kth);
extern int kth_sort(void *arr, size_t n, size_t size, int k, kthcmp cmp, void *kth);

#endif /* _KTH_H */

