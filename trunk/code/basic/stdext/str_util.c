/**
 *	@file	string_util.c
 *	@brief	safe string functions.
 *
 *	@author	Forrest.zhang
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "gcc_common.h"
#include "dbg_common.h"
#include "str_util.h"

typedef int (*str_cmp_func)(const char *s1, const char *s2, size_t n);

/**
 *	Get first '\0' in @str and save in @p 
 */
#define	STR_GET_TAIL(str, siz, p)		\
({						\
	p = str;				\
 	while (*p && p < (str + siz))		\
		p++;				\
})

int 
str_array_free(char *array[])
{
	char **p;

	if (unlikely(!array))
		ERR_RET(-1, "invalid argument\n");

	p = array;
	while (*p != NULL) {
		free(*p);
		p++;
	}

	free(array);
	return 0;
}

int
str_len(const char *str, size_t siz)
{
	register const char *p;

	if (unlikely(!str || siz < 1))
		ERR_RET(-1, "invalid argument\n");

	/* get @str end pos */
	STR_GET_TAIL(str, siz, p);

	/* count does not include NUL */
	return (p - str);
}

int
str_copy(const char *src, size_t ssiz, char *dst, size_t dsiz)
{
	register const char *p;

	if (unlikely(!dst || !src || dsiz < 1 || ssiz < 1))
		ERR_RET(-1, "invalid argument\n");

	/* get @src end pos */
	STR_GET_TAIL(src, ssiz, p);

	/* calc @src real length */
	ssiz = p - src;

	/* @dst is small hold all chars */
	if (unlikely(dsiz <= ssiz))
		ERR_RET(-1, "@dst is too small\n");

	/* Copy as many bytes as will fit */
	memcpy(dst, src, ssiz);

	/* zero end @dst */
	dst[ssiz]='\0';

	/* count does not include NUL */
	return ssiz;
}

int 
str_cat(const char *src, size_t ssiz, char *dst, size_t dsiz)
{
	register size_t n;
	register char *p;
	register const char *p1;

	/* get @dst end pos */
	STR_GET_TAIL(dst, dsiz, p);

	/* @dst is not zero-end */
	if (unlikely(p == dst + dsiz))
		ERR_RET(-1, "@dst is not zero end\n");

	/* calc @dst spare space */
	n = (dst + dsiz) - (p + 1);

	/* get @src end pos */
	STR_GET_TAIL(src, ssiz, p1);

	/* calc @src real length */
	ssiz = p1 - src;

	if (unlikely(n < ssiz))
		ERR_RET(-1, "@dst have small space\n");

	/* copy string */
	n = ssiz;
	memcpy(p, src, n);

	/* zero-end @dst */
	p[n] = 0;

	return n;
}

int 
str_cmp(const char *str1, size_t siz1, const char *str2, size_t siz2)
{
	register int ret;
	register size_t n;
	register const char *p1, *p2;

	if (unlikely(!str1 || !str2 || siz1 < 1 || siz2 < 1))
		ERR_RET(-1, "invalid argument\n");

	if (str1 == str2 && siz1 == siz2)
		return 0;

	/* get @str end pos */
	STR_GET_TAIL(str1, siz1, p1);

	/* calc @str1 real length */
	siz1 = p1 - str1;

	/* get @str2 end pos */
	STR_GET_TAIL(str2, siz2, p2);

	/* calc @str2 real length */
	siz2 = p2 - str2;

	n = siz1 > siz2 ? siz2 : siz1;
	ret = memcmp(str1, str2, n);

	return ret == 0 ? (siz1 - siz2) : ret;
}

int 
str_have_char(const char *str, size_t siz, int c)
{
	register const char *p;
	register const char *end;

	if (unlikely(!str || siz < 1))
		ERR_RET(-1, "invalid argument\n");

	/* get @str end pos */
	STR_GET_TAIL(str, siz, p);

	end = p;
	p = str;
	do {
		if (*p == c)
			return 1;
		p++;
	} while(p < end);

	return 0;
}

char *
str_chr(const char *str, size_t siz, int c)
{
	register const char *p;
	register const char *end;

	if (unlikely(!str || siz < 1))
		ERR_RET(NULL, "invalid argument\n");

	STR_GET_TAIL(str, siz, p);

	end = p;
	p = str;

	do {
		if (*p == c)
			return (char *)p;
		p++;
	} while (p < end);

	return NULL;
}

char *
str_rchr(const char *str, size_t siz, int c)
{
	register const char *p;

	if (unlikely(!str || siz < 1))
		ERR_RET(NULL, "invalid argument\n");

	STR_GET_TAIL(str, siz, p);

	do {
		if (*p == c)
			return (char *)p;
		p--;
	} while (p >= str);

	return NULL;
}

char * 
str_find1(const char *str, size_t siz, str_is_func func)
{
	register const char *p;
	register const char *end;

	if (unlikely(!str || siz < 1 || !func))
		ERR_RET(NULL, "invalid argument\n");

	/* get @str end pos */
	STR_GET_TAIL(str, siz, p);

	end = p;
	p = str;
	do {
		if (func(*p))
			return (char *)p;
		p++;
	} while(p < end);

	return NULL;
}

char *
str_rfind1(const char* str, size_t siz, str_is_func func)
{
	register const char *p;

	if (unlikely(!str || siz < 1 || !func))
		ERR_RET(NULL, "invalid argument\n");

	/* get @str end pos */
	STR_GET_TAIL(str, siz, p);

	do {
		if (func(*p) > 0)
			return (char *)p;
		p--;
	} while (p >= str);

	return NULL;
}

char * 
str_find3(const char *str, size_t siz, const char *chrs, size_t nchr, str_in_func func)
{
	register const char *p;
	register const char *end;

	if (unlikely(!str || siz < 1 || !chrs || nchr < 1 || !func))
		ERR_RET(NULL, "invalid argument\n");

	/* get @str end pos */
	STR_GET_TAIL(str, siz, p);

	end = p;
	p = str;
	do {
		if (func(chrs, nchr, *p) > 0)
			return (char *)p;
		p++;
	} while(p < end);

	return NULL;
}

char * 
str_rfind3(const char *str, size_t siz, const char *chrs, size_t nchr, str_in_func func)
{
	register const char *p;

	if (unlikely(!str || siz < 1 || !chrs || nchr < 1 || !func))
		ERR_RET(NULL, "invalid argument\n");

	/* get @str end pos */
	STR_GET_TAIL(str, siz, p);

	do {
		if (func(chrs, nchr, *p))
			return (char *)p;
		p--;
	} while(p >= str);

	return NULL;
}

char *
str_str(const char *str, size_t siz, const char *pat, size_t psiz, int case_sens)
{
	str_cmp_func cmp_func;
	register const char *p;
	register const char *end;

	/* invalid parameter */
	if (unlikely(!str || !pat || siz < 1 || psiz < 1))
		ERR_RET(NULL, "invalid argument\n");

	/* decide compare function */
	if (case_sens)
		cmp_func = strncmp;
	else
		cmp_func = strncasecmp;

	/* get @str end pos */
	STR_GET_TAIL(str, siz, p);

	/* calc @str real length */
	siz = p - str;

	/* get @pat end pos */
	STR_GET_TAIL(pat, psiz, p);

	/* calc @pat real length */
	psiz = p - pat;

	/* @pat large than @str */
	if (psiz > siz)
		return NULL;

	/* search @pat */
	p = str;
	end = str + siz - psiz;
	do {
		if (cmp_func(p, pat, psiz) == 0)
			return (char *)p;
		p++;
	} while (p <= end);

	/* not found */
	return NULL;
}

char *
str_rstr(const char *str, size_t siz, const char *pat, size_t psiz, int case_sens)
{
	str_cmp_func cmp_func;
	register const char *p;

	if (unlikely(!str || !pat || siz < 1 || psiz < 1))
		ERR_RET(NULL, "invalid argument\n");

	/* decide compare function */
	if (case_sens)
		cmp_func = strncmp;
	else
		cmp_func = strncasecmp;

	/* get @str end pos */
	STR_GET_TAIL(str, siz, p);

	/* calc @str real length */
	siz = p - str;

	/* get @pat end pos */
	STR_GET_TAIL(pat, psiz, p);

	/* calc @pat real length */
	psiz = p - pat;

	/* @pat large than @str */
	if (psiz > siz)
		return NULL;

	/* reverse search @pat */
	p = str + siz - psiz;
	do {
		if (cmp_func(p, pat, psiz) == 0)
			return (char *)p;
		p--;
	} while (p >= str);

	/* not found */
	return NULL;
}

char *
str_token(char *str, size_t siz, const char *delim, size_t dsiz, char **off)
{
	register char *p, *ptr;
	register const char *p1;
	register const char *end1;

	if (unlikely(!delim || !off || siz < 1 || dsiz < 1))
		ERR_RET(NULL, "invalid argument\n");

	/* get @str start pos */
	if (str) {
		/* get @str end pos */
		STR_GET_TAIL(str, siz, p);
		
		/* @str is not zero-end */
		if (unlikely(p == (str + siz)))
			ERR_RET(NULL, "@str is not zero end\n");

		ptr = str;
	}
	else {
		/* @*off is NULL, no next token */
		if (*off == NULL)
			return NULL;

		p = *off;
		ptr = *off;
	}

	end1 = delim + dsiz;
	while (*p) {
		p1 = delim;
		while (*p1 && p1 < end1) {
			if (*p == *p1) {
				*p = 0;
				*off = p + 1;
				return ptr;
			}
			p1++;
		}
		p++;
	}

	/* not match delim, next call will return NULL */
	*off = NULL;
        return str;
}

int 
str_strip(const char *src, size_t ssiz, char *dst, size_t dsiz, str_is_func func)
{
	register const char *end;
	register const char *begin;

	if (!dst || dsiz < 1 || !src || ssiz < 1 || !func)
		ERR_RET(-1, "invalid argument\n");

	begin = str_find1(src, ssiz, func);
	if (!begin)
		return -1;

	end = str_rfind1(src, ssiz, func);
	if (!end)
		return -1;

	ssiz = (end - begin) + 1;

	if (dsiz <= ssiz)
		ERR_RET(-1, "@dst have small room\n");

	memcpy(dst, begin, ssiz);

	/* zero end @dst */
	dst[ssiz] = 0;

	return 0;
}


void *
mem_chr(const void *ptr, size_t siz, int c)
{
	register const char *p;
	register const char *end;

	if (unlikely(!ptr || siz < 1))
		ERR_RET(NULL, "invalid argument\n");

	p = ptr;
	end = ptr + siz;
	while (p < end) {
		if (*p == c)
			return (void *)p;
		p++;
	}

	return NULL;
}

void *
mem_rchr(const void *ptr, size_t siz, int c)
{
	register const char *p;
	register const char *begin;

	if (unlikely(!ptr || siz < 1))
		ERR_RET(NULL, "invalid argument\n");

	begin = ptr;
	p = ptr + siz - 1;
	while (p >= begin) {
		if (*p == c)
			return (void *)p;
		p--;
	}

	return NULL;
}

void * 
mem_find1(const void *ptr, size_t siz, str_is_func func)
{
	register const char *p;
	register const char *end;

	if (unlikely(!ptr || siz < 1 || !func))
		ERR_RET(NULL, "invalid argument\n");

	p = ptr;
	end = ptr + siz;
	while (p < end) {
		if (func(*p) > 0)
			return (void *)p;
		p++;
	}

	return NULL;
}

void *
mem_rfind1(const void *ptr, size_t siz, str_is_func func)
{
	register const char *p;
	register const char *begin;

	if (unlikely(!ptr || siz < 1 || !func))
		ERR_RET(NULL, "invalid argument\n");

	begin = ptr;
	p = ptr + (siz - 1);
	while (p >= begin) {
		if (func(*p) > 0)
			return (void *)p;
		p--;
	}

	return NULL;
}

void * 
mem_find3(const void *ptr, size_t siz, const char *chrs, size_t nchr, str_in_func func)
{
	register const char *p;
	register const char *end;

	if (unlikely(!ptr || siz < 1 || !chrs || nchr < 1 || !func))
		ERR_RET(NULL, "invalid argument\n");

	p = ptr;
	end = ptr + siz;
	while (p < end) {
		if (func(chrs, nchr, *p) > 0)
			return (void *)p;
		p++;
	}

	return NULL;

}

void * 
mem_rfind3(const void *ptr, size_t siz, const char *chrs, size_t nchr, str_in_func func)
{
	register const char *p;
	register const char *begin;

	if (unlikely(!ptr || siz < 1 || !chrs || nchr < 1 || !func))
		ERR_RET(NULL, "invalid argument\n");

	begin = ptr;
	p = ptr + (siz - 1);
	while (p >= begin) {
		if (func(chrs, nchr, *p) > 0)
			return (void *)p;
		p--;
	}

	return NULL;
}


