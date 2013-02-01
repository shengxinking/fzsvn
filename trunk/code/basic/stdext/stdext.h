/**
 *	@file	stdext.h
 *
 *	@brief	the extension APIs for standard library
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_STDEXT_H
#define FZ_STDEXT_H

/**
 *	Copy string @str to buffer @buf, the @buf length is @len.
 *	The @buf is zero-end after copy.
 *
 *	Return copied char number if success, -1 on error.
 */
extern size_t 
str_copy(char *buf, size_t len, const char *str);

/**
 *	Append string @src into string @dst, the @dst length is @len.
 *	The @dst is zero-end after append.
 *	
 *	Return the copied char number if success, -1 on error.
 */
extern size_t 
str_cat(char *dst, size_t len, const char *src);

/**
 *	Find character @c in string @str.
 *
 *	Return pointer to first find @c, NULL if not found
 */
extern char *  
str_char(const char *str, size_t len, char c);

/**
 *	Find character @c in string @str by reverse sequence.
 *
 *	Return pointer to first find @c, NULL if not found
 */
extern char *
str_rchar(const char *str, size_t len, char c);

/**
 *	Find the substring @substr in string @str, the @str
 *	length is @len.
 *
 *	Return pointer to found substring in @str if found.
 *	NULL if not found or error.
 */
extern char *
str_str(const char *str, size_t len, const char *substr);


/**
 *	Find the substring @substr in string @str in reverse orde, 
 *	the @str length is @len.
 *
 *	Return pointer to found substring in @str if found.
 *	NULL if not found or error.
 */
extern char * 
str_rstr(const char *str, size_t len, const char *substr);

/**
 *	Split the string @str into several substring by separeator
 *	@delim.
 *	It will malloc memory for splited strings.
 *
 *	Return the splited strings if success, NULL on not found.
 */
extern char ** 
str_split(const char *str, size_t len, const char *delim);

/**
 *	Freed the malloced memory alloced by @str_split
 *
 *	No return.
 */
extern void 
str_free(char **str);


#endif /* end of FZ_  */


