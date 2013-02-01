/**
 *	@file	strutil.c
 *	@brief	some functions manipulate string
 *
 *	@author	Forrest.zhang
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "strutil.h"


/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t str_cpy(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* verify params */
	if (!dst || !src)
		return 0;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NULL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';              /* NUL-terminate dst */
		while (*s++);
	}

	return(s - src - 1);    /* count does not include NUL */
}

/*
 * find char @c in string @s, if find, return it postion
 * else return NULL.
 */
char *str_chr(const char *s, char c)
{
	register const char *p = s;

	if (!s || c == '\0')
		return NULL;

	while (*p != '\0' && *p != c)
		p++;

	if (*p == c)
		return (char*)p;
	else
		return NULL;
}


/*
 * split @src to words according @delim, and return word array
 * user must free the result by str_free
 */
char **str_split(const char *src, const char *delim, int *size)
{
	register const char *s = src;
	register const char *e = src;
	register char *w = NULL;
	int i, n, m;
	char **words = NULL;
	char **tmp = NULL;
	char *ptr = NULL;

	if (!s || *s == 0)
		return NULL;

	i = 0;
	m = 0;
	while (*s) {
		/* skip leading space and delim */
		if (delim) {
			while (*s && (isspace(*s) || str_chr(delim, *s)))
				s++;
		}
		else {
			while (*s && isspace(*s))
				s++;
		}

		if (*s == 0)
			break;

		/* get the word end position */
		e = s;
		if (delim) {
			while (*e && !(isspace(*e) || str_chr(delim, *e)))
				e++;
		}
		else {
			while (*e && !isspace(*e))
				e++;
		}

		n = e - s;
		if (n == 0)
			break;

		/* alloc memory store words pointer, each time allocate another 
		 * 10 free space to avoid many malloc times.
		 */
		if (m == 0 || i == m - 1) {
			m += 10;
			tmp = malloc(sizeof(char *) * m);
			if (!tmp) {
				str_free(words);
				return NULL;
			}
			memset(tmp, 0, sizeof(char *) * m);

			/* copy old data to new postion */
			if (words) {
				memcpy(tmp, words, i * sizeof(char *));
				free(words);
			}

			words = tmp;
		}

		/* alloc memory store this word */
		ptr = malloc(n + 1);
		if (!ptr) {
			str_free(words);
			return NULL;
		}
		memset(ptr, 0, n + 1);
		w = ptr;
		while (s < e)
			*w++ = *s++;

		words[i] = ptr;
		i++;
	}

	if (size)
		*size = i;

	return words;
}

void str_free(char **str)
{
	char **ptr = NULL;

	if (!str)
		return;

	ptr = str;
	while (*ptr)
		free(*ptr++);

	free(str);
}

/*
 * split @src to two words stored in @str1, @str2 if it have two words.
 * If only have one, only stored in @str1, if have more than one, only
 * store first two words. If none, nothing to do.
 * return 0 if OK, -1 on error
 */
int str_split_dword(const char *src, char *str1, size_t siz1, char *str2, size_t siz2)
{
	register const char *s = src;
	register char *d1 = str1;
	register char *d2 = str2;
	register size_t n1 = siz1;
	register size_t n2 = siz2;

	/* verify param */
	if (!src)
		return -1;

	/* skip leading space */
	while (isspace(*s))
		s++;
	if (*s == 0)
		return 0;

	/* copy first word */
	if (n1 != 0 && --n1 != 0) {
		do {
			*d1++ = *s++;
		} while (--n1 != 0 && !isspace(*s) && (*s != 0));
	}
	
	/* not enough space */
	if (siz1 != 0)
		*d1 = 0;

	while (isspace(*s))
		s++;
	if (*s == 0)
		return 0;
	
	/* copy second word */
	if (n2 != 0 && --n2 != 0) {
		do {
			*d2++ = *s++;
		} while (--n2 != 0 && !isspace(*s) && (*s != 0));
	}

	if (siz2 != 0)
		*d2 = 0;

	return 0;
}


