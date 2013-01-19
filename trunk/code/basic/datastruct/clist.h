/*
 *	clist.h:	list using array implement, it avoid malloc every time.
 *
 *	author:		forrest.zhang
 */

#ifndef _FZ_CLIST_H
#define _FZ_CLIST_H

#include <sys/types.h>

typedef struct cnode {
	int	next;
	char	data[0];
} cnode_t;

typedef struct clist {
	size_t	size;
	size_t	objsize;
	int	count;
	int	next;
	char	data[0];
} clist_t;

typedef int (clist_cmp_func*)(const void *data1, const void *data2);
typedef void (clist_print_func*)(const void *data);

extern clist_t *alist_create(size_t objsize, size_t maxobjs);
extern int clist_count(const clist_t *list);
extern int clist_empty(const clist_t *list);
extern int clist_remain(const clist_t *list);

extern int clist_find(const clist_t *list, const void *data, clist_cmp_func cmp);
extern int clist_find_prev(const clist_t *list, const void *data, clist_cmp_func cmp);

extern int clist_add_prev(const clist_t *list, const void *data, int pos);
extern int clist_add_next(const clist_t *list, const void *data, int pos);
extern int clist_add_head(const clist_t *list, const void *data);
extern int clist_add_tail(const clist_t *list, const void *data);

extern int clist_del(clist_t *list, const void *data);
extern int clist_head(clist_t *list);
extern int clist_tail(clist_t *list);

extern int clist_print(const clist_t *list, clist_print_func print);

#endif /* end of _FZ_CLIST_ARR_H */

