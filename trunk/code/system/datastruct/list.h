/*
 *	list.h:		the head file of list data struct, it using pointer 
 *			implement index
 *
 * 	author:		forrest.zhang
 */

#ifndef FZ_LIST_H
#define FZ_LIST_H

#include <sys/types.h>

typedef int (*list_cmp_func)(const void *data1, const void *data2);
typedef int (*list_find_func)(const void *data1, const void *data2);
typedef void (*list_print_func)(const void *data);
typedef void (*list_free_func)(void *data);
typedef int (*list_iterator_func)(void *data, int pos);	/* this function is used to iterator all list */

typedef struct list {
	list_cmp_func	cmp_func;
	list_find_func	find_func;
	list_print_func print_func;
	list_free_func	free_func;
	u_int32_t	size;
	u_int32_t	pos;
	void		*front;
	void		*rear;
} list_t;

extern list_t *list_alloc(list_cmp_func cmp_func, list_find_func find_func,
			list_print_func print_func, list_free_func free_func);
extern void list_free(list_t *list);

extern int list_empty(const list_t *list);
extern int list_count(const list_t *list);

extern void *list_find(const list_t *list, void *data);
extern void *list_get(const list_t *list, int pos);
extern int list_iterator(list_t *list, list_iterator_func iterator_func);

extern int list_add_next(list_t *list, int pos, void *data);
extern int list_add_prev(list_t *list, int pos, void *data);
extern int list_add_tail(list_t *list, void *data);
extern int list_add_head(list_t *list, void *data);

extern int list_del(list_t *list, int pos);
extern int list_del_head(list_t *list);
extern int list_del_tail(list_t *list);

extern int list_add_asc(list_t *list, void *data);
extern int list_add_dsc(list_t *list, void *data);
extern list_t *list_sort_asc(const list_t *list);
extern list_t *list_sort_dsc(const list_t *list);

extern void list_print(const list_t *list);

#endif /* end of FZ_LIST_H */

