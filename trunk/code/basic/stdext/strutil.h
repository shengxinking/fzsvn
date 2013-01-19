/*
 * @file	strutil.h
 * @brief	some string manipulate functions
 *
 * @author	Forrest.zhang
 */

#ifndef FS_STRUTIL_H
#define FS_STRUTIL_H

#include <sys/types.h>

#include "declare.h"


BEGIN_DECLS

/* it's a safe string copy function */
extern size_t str_cpy(char *dst, const char *src, size_t siz);

/** it's a safe string split function, it split @src to two words
 *  stored in @str1, @str2 if exist two words, it's useful to read 
 *  some text file */
extern int str_split_dword(const char *src, char *str1, size_t siz1, char *str2, size_t siz2);

extern char *str_chr(const char *s, char c);
extern char **str_split(const char *src, const char *delim, int *size);
extern void str_free(char **str);

END_DECLS

#endif /* end of FS_STRUTIL_H */

