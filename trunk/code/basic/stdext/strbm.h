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
 *	pattern @pattern, the charactoer array should
 *	be 256 bytes for unsigned char, 128 for ASCII
 *	charactor.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
strbm_bc(const char *pattern, int plen, int *bc, int bclen);


/**
 *	Generator good suffix array according the 
 *	pattern @pattern, the @gslen should equal 
 *	the 
 *
 */
extern int 
strbm_gs(const char *pattern, int plen, int *gs, int gslen);


/**
 *
 *
 *
 */
extern char * 
strbm_search(const char *buf, int blen, const char *pat, int plen, 
	     const int *bc, int bclen, const int *gs, int gslen);



#endif /* end of FZ_STRBM_H  */

