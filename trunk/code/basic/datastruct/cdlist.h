/*
 *	cdlist.h:	using array implement double-linked list
 *
 *	author:		forrest.zhang
 */

#ifndef _FZ_CDLIST_H
#define _FZ_CDLIST_H

typedef struct adnode {
	int	prev;
	int	next;
	char	data[0];
} adnode_t;

typedef struct adlist {
	size_t	size;
	size_t	objsize;
	size_t	count;
	int	head;
	int	tail;
	char	data[0];
} adlist_t;

typedef int (adlist_cmp_func*)(const void *data1, const void *data2);
typedef void (adlist_print_func*)(const void *data);

extern adlist_t *adlist_create(size_t objsize, size_t maxobjs);

extern int adlist_empty(const adlist_t *list);
extern int adlist_count(const adlist_t *list);
extern int adlist_remain(const adlist_t *list);

extern int adlist_find(const adlist_t *list, const void *data);

extern int adlist_add_prev(adlist_t *list, const void *data, int pos);
extern int adlist_add_next(adlist_t *list, const void *data, int pos);
extern int adlist_add_head(adlist_t *list, const void *data);
extern int adlist_add_tail(adlist_t *list, const void *data);

extern int adlist_del(adlist_t *list, const void *data, adlist_cmp_func cmp);
extern int adlist_del_tail(adlist_t *list);
extern int adlist_del_head(adlist_t *list);

extern int adlist_print(const adlist_t *list, adlist_print_func print);

#endif /* end of _FZ_CDLIST_H */

