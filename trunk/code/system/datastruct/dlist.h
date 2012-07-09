/*
 *	list.h:		the head file of list data struct, it using pointer 
 *			implement index
 *
 * 	author:		forrest.zhang
 */

#ifndef _FZ_DLIST_H
#define _FZ_DLIST_H

typedef struct dnode {
	void		*data;
	struct dnode	*next;	
	struct dnode	*prev;
} dnode_t;

typedef struct dlist {
	int		size;
	dnode_t		*head;
	dnode_t		*tail;
} dlist_t;

typedef int (*dlist_cmp_func)(const void *data1, const void *data2);
typedef void (*dlist_print_func)(const void *data);

extern dlist_t *dlist_create(void);
extern int    dlist_empty(const dlist_t *list);
extern int    dlist_count(const dlist_t *list);

extern dnode_t *dlist_find(const dlist_t *list, const void *data, dlist_cmp_func cmp);
extern dnode_t *dlist_rfind(const dlist_t *list, const void *data, dlist_cmp_func cmp);
extern dnode_t *dlist_get(const dlist_t *list, int pos);

extern int dlist_add_next(dlist_t *list, dnode_t *node, int pos);
extern int dlist_add_prev(dlist_t *list, dnode_t *node, int pos);

extern dnode_t *dlist_del(dlist_t *list, const void *data, dlist_cmp_func cmp);
extern dnode_t *dlist_del_tail(dlist_t *list);
extern dnode_t *dlist_del_head(dlist_t *list);

extern void dlist_print(const dlist_t *list, dlist_print_func print);
extern void dlist_rprint(const dlist_t *list, dlist_print_func print);

//extern list_t list_sort_asc(list_t list, list_cmp cmp);
//extern list_t list_sort_dsc(list_t list, list_cmp cmp);

//extern list_t list_add_asc(list_t list, node_t *node);
//extern list_t list_add_dsc(list_t list, node_t *node);


#endif /* end of _FZ_DLIST_H */

