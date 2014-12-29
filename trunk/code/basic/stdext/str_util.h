/**
 *	@file	str_util.h
 *
 *	@brief	safe string functions for replace glibc 
 *		strxx() functions.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_STR_UTIL_H
#define FZ_STR_UTIL_H

/*----------------------------------------------------------
 |	String array:
 |      ptr1 --> string1
 |      ptr2 --> string2
 |	..............
 |	ptrN --> NULL(end of array)
 |
 |--------------------------------------------------------*/

/**
 *	The common character check function, 
 *	See <ctype.h> functions isxxxx().
 *
 *	Return 1 if matched, 0 if not.
 */
typedef int (*str_is_func)(int c);

/**
 *	Check @c is in @str or not. @str size is @siz
 *	
 *	Return 1 if @c in @str, 0 if not, -1 on error.
 */
typedef int (*str_in_func)(const char *str, size_t siz, int c);

/**
 *	Free string array @str and all alloced memory 
 *	inside it.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
str_array_free(char **array);

/**
 *	Return string @str real length. @str size is @siz.
 *
 *	Return >= 0 is success, -1 on error or @str is 
 *	not zero-end.
 */
extern int 
str_len(const char *str, size_t siz);

/**
 *	Copy string @src to destination string @dst, the 
 *	@dst size is @dsiz. The @src size is @ssiz. 
 *	@dst is zero-end after copy.
 *
 *	Return copied char number if success, -1 on error 
 *	or @dst space is small.
 */
extern int  
str_copy(const char *src, size_t ssiz, char *dst, size_t dsiz);

/**
 *	Append string @src to string @dst, the @dst size
 *	is @dsiz, the @src size is @ssiz.
 *	The @dst is zero-end after append.
 *	
 *	Return the copied bytes number if success, -1 error 
 *	or @dst space is small.
 */
extern int 
str_cat(const char *src, size_t ssiz, char *dst, size_t dsiz);

/**
 *	Compare string @str1 and @str2. @str1 size is @siz1,
 *	@str2 size is @siz2.
 *
 *	Return 0 if them have same value, 
 *	or else not same value.
 */
extern int 
str_cmp(const char *str1, size_t siz1, const char *str2, size_t siz2);

/**
 *	Find first char @c in string @str, @str size is @siz.
 *
 *	Return pointer to first @c, NULL not found or error
 *	or @str is not zero-end.
 */
extern char *  
str_chr(const char *str, size_t siz, int c);

/**
 *	Find last char @c in string @str, @str length is 
 *	@len
 *
 *	Return pointer to last @c, NULL if not found or 
 *	error or @str is not zero-end.
 */
extern char *
str_rchr(const char *str, size_t len, int c);

/**
 *	Check char @c exist in @str, @str size is @siz.
 *
 * 	Return 1 if @c in @str, 0 not in or error.
 */
extern int 
str_has_char(const char *str, size_t siz, int c);

/**
 * 	Find first character in @str which call @func() return 
 * 	non-zero, @str size is @siz.
 *
 * 	Return non NULL pointer if success, NULL 
 * 	on error or not found.
 */
extern char *
str_find1(const char *str, size_t len, str_is_func func);

/**
 * 	Find last characters which call @func() return non-zero 
 * 	in @str and return pointer it. @str size is @siz.
 *
 * 	Return non NULL pointer, NULL on error or not found.
 */
extern char * 
str_rfind1(const char *str, size_t siz, str_is_func func);

/**
 * 	Find first characters which call @func() return no-zero
 * 	in @str and return pointer it.
 *
 * 	Return non NULL pointer, NULL on error or not found.
 */
extern char *
str_find3(const char *str, size_t siz, const char *chrs, size_t nchr, str_in_func func);

/**
 * 	Find last characters which call @func() return non-zero
 * 	in @str and return pointer it.
 *
 * 	Return non NULL pointer, NULL on error or not found.
 */
extern char * 
str_rfind3(const char *str, size_t siz, const char *chrs, size_t nchr, str_in_func func);

/**
 *	Find the first substring @pat in string @str, 
 *	@str size is @siz, @pat size is @psiz. 
 *	If @case_sens is non-zero use case-sensitive compare,
 *	or else using non-case sensitive compare function.
 *
 *	Return pointer to found substring in @str if found.
 *	NULL if not found or error or @str is not 
 *	zero-end.
 */
extern char *
str_str(const char *str, size_t siz, const char *pat, size_t psiz, int case_sens);

/**
 *	Find the last substring @substr in string @str, 
 *	@str length is @len, @substr length is @slen
 *	If @case_sens is non-zero use case-sensitive compare,
 *	or else using non-case sensitive compare function.
 *
 *	Return pointer to found substring in @str if found.
 *	NULL if not found or error or @str is not 
 *	zero-end.
 */
extern char *
str_rstr(const char *str, size_t len, const char *pat, size_t psiz, int case_sens);

/**
 *	Strip @src begin/end characters which call @func return @val 
 *	and save it into @dst. @dst length is @dlen.
 *
 * 	dlen must >= @slen.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
str_strip(const char *src, size_t ssiz, char *dst, size_t dsiz, str_is_func func);


/**
 *	Find first unsigned char @c in memory @ptr.
 *
 *	Return address to find @c if success, NULL
 *	on error or not found.
 */
extern void * 
mem_chr(const void *ptr, size_t siz, int c);

/**
 *	Find last unsigned char @c in memory @ptr.
 *
 *	Return address to find @c if success, NULL
 *	on error or not found.
 */
extern void * 
mem_rchr(const void *ptr, size_t siz, int c);

/**
 * 	Find first character which call @func() >0
 * 	in @ptr and return pointer it.
 *
 * 	Return non NULL pointer, NULL on error or not found.
 */
extern void * 
mem_find1(const void *ptr, size_t siz, str_is_func func);

/**
 * 	Find last characters which call @func() >0
 * 	in @ptr and return pointer it.
 *
 * 	Return non NULL pointer, NULL on error or not found.
 */
extern void * 
mem_rfind1(const void *ptr, size_t siz, str_is_func func);

/**
 * 	Find first character which call @func(@chrs, @nchr, *p) > 0
 * 	in @ptr and return pointer it.
 *
 * 	Return non NULL pointer, NULL on error or not found.
 */
extern void * 
mem_find3(const void *ptr, size_t siz, const char *chrs, size_t nchr, str_in_func func);

/**
 * 	Find last character which call @func(@chrs, @nchr, *p) > 0 
 * 	in @ptr and return pointer it.
 *
 * 	Return non NULL pointer, NULL on error or not found.
 */
extern void * 
mem_rfind3(const void *ptr, size_t siz, const char *chrs, size_t nchr, str_in_func func);

#endif /* end of FZ_STR_UTIL_H  */


