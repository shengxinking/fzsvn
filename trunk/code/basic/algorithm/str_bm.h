/**
 *	@file	str_bm.h
 *
 *	@brief	Using Boyer-Moore algorithm implement string find.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2011-10-13
 */

#ifndef FZ_STRBM_H
#define FZ_STRBM_H

/**
 *	Generator bad charactor array according the 
 *	pattern @pat, the character array should
 *	be 256 bytes for unsigned char, 128 for ASCII
 *	charactor.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
str_bm_bc(const char *pat, int plen, int *bc, int bclen);


/**
 *	Generator good suffix array according the 
 *	pattern @pat, the @gslen should equal 
 *	the @plen
 *
 * 	Return 0 if success.
 */
extern int 
str_bm_gs(const char *pat, int plen, int *gs, int gslen);


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
str_bm_search(const char *str, size_t slen, 
	     const char *pat, size_t plen, 
	     const int *bc, size_t bclen, 
	     const int *gs, size_t gslen);


#endif /* end of FZ_STRBM_H  */

