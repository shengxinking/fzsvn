/**
 *	@file
 *
 *	@brief
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_ALGORITHM_H
#define FZ_ALGORITHM_H

#include <sys/types.h>

/**
 *	Binary compare function. If @a == @b return 0, 
 *	@a > @b return >0 value, @a < @b return <0 value.
 */
typedef int (*bin_search_cmp_func)(const void *a, const void *b);


/*
 *	bsearch:	search @val in a ascend sorted array @array.
 *	@arr		ascend array
 *	@objsize	object size
 *	@nobjs		number of objects in array
 *	@cmp		function pointer using compare two elements
 *	@val		the value which need search
 *
 * 	return a pointer to the element if OK, NULL on error.
 */
extern const void * 
bin_search(const void *array, size_t objsize, size_t nobj, 
	   bin_search_cmp_func cmp, const void *val);

/**
 *	Generator bad charactor array according the 
 *	pattern @pat, the character array should
 *	be 256 bytes for unsigned char, 128 for ASCII
 *	charactor.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
BM_get_bc(const char *pat, size_t plen, int *bc, size_t bclen);


/**
 *	Generator good suffix array according the 
 *	pattern @pat, the @gslen should equal 
 *	the @plen.
 *
 * 	Return 0 if success.
 */
extern int 
BM_get_gs(const char *pat, size_t plen, int *gs, size_t gslen);


/**
 *	Search string @pat in string @str using BM algorithm, 
 *	the @str length is @slen, @pat length is @plen, 
 *	the bad character array is @bc, @bc length is @bclen, 
 *	the good suffix array is @gs, the @gs length is @gslen.
 *
 *	Return pointer to @pat in @str if found, NULL not found
 *	or error.
 */
extern char * 
BM_search(const char *str, size_t slen, 
	  const char *pat, size_t plen, 
	  const int *bc, size_t bclen, 
	  const int *gs, size_t gslen);

/**
 *	Return the GCD value of @a and @b.
 *
 *	Return the gcd value.
 */
extern unsigned long 
gcd(unsigned long a, unsigned long b);


#endif /* end of FZ_ALGORITHM_H  */

