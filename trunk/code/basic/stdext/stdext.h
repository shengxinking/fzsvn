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
 *	Define likely()/unlikely() for performance.
 */
#ifndef	likely
#define	likely(x)	__builtin_expect(!!(x), 1)
#endif

#ifndef	unlikely
#define	unlikely(x)	__builtin_expect(!!(x), 0)
#endif


/***********************************************************
 *	String array (char **):
 *
 *	char *str1
 *	char *str2
 *	..........
 *	char *strN
 *	NULL
 *
 ***********************************************************/

/**
 *	Copy string @str to buffer @buf, the @buf length 
 *	is @len. The @buf is zero-end after copy.
 *
 *	Return copied char number if success, -1 on error.
 */
extern size_t 
str_copy(char *buf, size_t len, const char *str);

/**
 *	Append string @src into string @dst, the @dst length
 *	is @len. The @dst is zero-end after append.
 *	
 *	Return the copied bytes number if success, -1 error.
 */
extern size_t 
str_cat(char *dst, size_t len, const char *src);

/**
 *	Find first char @c in string @str.
 *
 *	Return pointer to first @c, NULL not found
 */
extern char *  
str_char(const char *str, size_t len, char c);

/**
 *	Find last char @c in string @str.
 *
 *	Return pointer to last @c, NULL if not found
 */
extern char *
str_rchar(const char *str, size_t len, char c);

/**
 *	Find the first substring @substr in string @str, 
 *	the @str length is @len. If @case is non-zero
 *	using case-sensitive compare, or else using 
 *	case-insensitive compare
 *
 *	Return pointer to found substring in @str if found.
 *	NULL if not found or error.
 */
extern char *
str_str(const char *str, size_t len, const char *substr, int case);

/**
 *	Find the last substring @substr in string @str, 
 *	the @str length is @len. If @case is non-zero
 *	using case-sensitive compare, or else using 
 *	case-insensitive compare
 *
 *	Return pointer to found substring in @str if found.
 *	NULL if not found or error.
 */
extern char * 
str_rstr(const char *str, size_t len, const char *substr, int case);

/**
 *	Find all of @substr in string @str, the @str length 
 *	is @len. If @case is non-zero using case-sensitive 
 *	compare, or else using case-insensitive compare
 *
 *	Need call @str_array_free() free returned string
 *	array.
 *
 *	Return string array of all substring in @str 
 *	if success, NULL if not found or error.
 */
extern char ** 
str_strs(const char *str, size_t len, const char *substr, int case);

/**
 *	Split the string @str into string array by 
 *	separeator @delim. 
 *
 *	Need call @str_array_free() free returned string
 *	array.
 *
 *	Return the splited strings if success, NULL on not found.
 */
extern char ** 
str_split(const char *str, size_t len, const char *delim);

/**
 *	Freed the string array @strs
 *
 *	No return.
 */
extern void 
str_array_free(char **strs);

/**
 *	Compare 2 strings @str1 and @str2, the @str1 max length 
 *	is @len1, the @str2 max length is @len2
 *
 *	Return -1 if @str1 < @str2, 0 if @str1 = @str2, 1 if
 *	@str1 > @str2.
 */
extern int 
str_cmp(const char *str1, size_t len1, const char *str2, 
	size_t len2, int case);

/**
 *	Remove all files/directories in directory @dir except 
 *	files/directory in string array @execptions.
 *
 *	Return if success, -1 on error.
 */
extern int 
dir_empty(const char *dir, const char *exceptions);

/**
 *	Create a new directory @dir, if the ancient path of @dir
 *	is not exist, also created. All new create directory mode
 *	is @mode.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dir_create(const char *dir, int mode);

/**
 *	Remove directory @dir and files/directories in it.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dir_remove(const char *dir);

/**
 *	Copy directory @srcdir and all files/directories in 
 *	it to directory @dstdir, if @dstdir not exist, 
 *	create it.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
dir_copy(const char *dstdir, const char *srcdir);

/**
 *	Get all files in directory @dir.
 *
 *	Need free the string array after use.
 *
 *	Return string array of files if success, -1 on error.
 */
extern char ** 
dir_files(const char *dir);

/**
 *	Get all directories in directory @dir
 *
 *	Need free the string array after use.
 * 
 *	Return string array of directories if success, -1 on error.
 */
extern char ** 
dir_dirs(const char *dir);

#endif /* end of FZ_STDEXT_H  */


