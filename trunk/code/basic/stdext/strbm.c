/**
 *	@file	str_bm.c
 *
 *	@brief	the Boyer Morre algorithm implement for string search. 
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2011-10-13
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "strbm.h"


int 
strbm_bc(const char *pat, int plen, int *bc, int bclen)
{
	int i;

	if (!pat || !bc)
		return -1;

	if (bclen < 1 || bclen < 256)
		return -1;

	for (i = 0; i < 256; i++)
		bc[i] = plen;

	for (i = 0; i < plen - 1; i++)
		bc[(unsigned int)pat[i]] = plen - (i + 1);

	return 0;
}



int 
strbm_gs(const char *pat, int plen, int *gs, int gslen)
{
	char *pend;
	char *pptr;
	int *gend;
	unsigned char c;

	if (!pat || !gs)
		return -1;

	if (plen < 1 || gslen < plen)
		return -1;

	pend = (char *)pat + plen - 1;
	gend = gs + plen - 1;

	*gend = 1;
	pptr = (char *)pat + plen - 2;

	/* save the last charactor */
	c = (unsigned char)*pend;

	/* from @pat end to start, find the equal string as suffix */
	while (gend-- != gs) {

		char *p1 = (char *)pat + plen - 2;
		char *p2, *p3;

		do {			
			/* find the first character same as the end char */
			while (p1 >= pat && *p1-- != c);
			
			p2 = (char *)pat + plen - 2;
			p3 = p1;

			/* find the match string [@pptr:end of @pat] */
			while(p3 >= pat && *p3-- == *p2-- && p2 >= pptr);

		} while (p3 >= pat && p2 >= pptr);

		*gend = p2 - p3;

		pptr--;
	}

	return 0;
}


char * 
strbm_search(const char *buf, int blen, const char *pat, int plen, 
	     const int *bc, int bclen, const int *gs, int gslen)
{
	int b_idx;
	int p_idx;
	int skip;
	int shift;
	int offset;
	
	if (!buf || !pat || !bc || !gs)
		return NULL;

	if (bclen < 256 || gslen < plen)
		return NULL;

	if (blen < 1 || plen < 1)
		return NULL;

	b_idx = plen;
	while (b_idx <= blen) {
		p_idx = plen;
//		printf("b_idx is %d\n", b_idx);
		/* matched charactor */
		while (buf[--b_idx] == pat[--p_idx]) {
//			printf("b_idx %d is same\n", b_idx);
			if (b_idx < 0)
				return NULL;

			if (p_idx == 0)
				return (char *)buf + b_idx;
		}

		/* calc the bad charactor skip length */
		skip = bc[(unsigned char)buf[b_idx]];

		/* calc the good suffix shift length */
		shift = gs[p_idx];

		offset = plen - p_idx;

		/* using max length of @skip and @shift */
//		b_idx += (skip > shift) ? skip : shift;
		b_idx += skip;
//		b_idx += (shift + plen - p_idx);
		b_idx += offset;
//		printf("b_idx is %d, skip is %d, offset is %d\n", b_idx, skip, offset);
	}

	return NULL;
}







