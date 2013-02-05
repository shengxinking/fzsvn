/*
 *	list.h:		the head file of list data struct, it using pointer 
 *			implement index
 *
 * 	author:		forrest.zhang
 */

#ifndef FZ_LIST_H
#define FZ_LIST_H

#include <sys/types.h>

#define	LIST_HEAD(name) \
	struct list_head name = {.head = NULL, .tail = NULL, .size = 0}

/**
 * 	List node structure.
 */
typedef struct list_node {
	struct list_node	*next;	/* next element */
	struct list_node	*prev;	/* previous element */
} list_node_t;

/**
 * 	List header structure.
 */
typedef struct list_head {
	list_node_t		*head;	/* the first element */
	list_node_t		*tail;	/* the last element */
	size_t			size;	/* number of element */
} list_head_t;

/**
 * 	Hlist node structure.
 */
typedef struct hlist_node {
	struct hlist_node	*next;	/* the next object */
	struct hlist_node	**prev;	/* the previous object address */
} hlist_node_t;

/**
 * 	Hlist header struct.
 */
typedef struct hlist_head {
	struct hlist_node	*first;	/* the first object in hash list */
	size_t			size;
} hlist_head_t;

/**
 * 	Add a element @item to list @list's as it first element. 
 */
static inline void 
list_add_head(list_head_t *list, list_node_t *item)
{
	item->prev = NULL;
	item->next = list->head;
	list->head = item;
	if (item->next == NULL)
		list->tail = item;
	item->size++;
}

/**
 * 	Add a element @item to list @list as it's last element.
 */
static inline void 
list_add_tail(list_head_t *list, list_node_t *item)
{
	item->prev = list->tail;
	item->next = NULL;
	list->tail = item;
	if (item->prev == NULL)
		list->head = item;
	item->size++;
}

/**
 *	Add a element @item to list @list, it's position is before
 *	element @old.
 */
static inline void 
list_add_before(list_head_t *list, list_node_t *old, list_node_t *item)
{
	item->next = old;
	item->prev = old->prev;
	old->prev = item;
	if (item->prev)
		item->prev->next = item;
	else
		list->head = item;
	list->size++;
}

/**
 * 	Add a element @item to list @list, it's position is after
 * 	element @old.
 */
static inline void 
list_add_after(list_head_t *list, list_node_t *old, list_node_t *item)
{
	item->prev = old;
	item->next = old->next;
	old->next = item;
	if (item->next)
		item->next->prev = item;
	else
		list->tail = item;
	list->size++;
}

/**
 * 	Delete element @item from list @list.
 */
static inline void 
list_del(list_head_t *list, list_node_t *item)
{
	if (item->next)
		item->next->prev = item->prev;
	else
		list->tail = item->prev;
	if (item->prev)
		item->prev->next = item->next;
	else
		list->head = item->next;
	list->size--;
	item->next = NULL;
	item->prev = NULL;
}

/**
 * 	Replace element @old using new element @item in list @list.
 */
static inline void 
list_replace(list_head_t *list, list_node_t *old, list_head_t *item)
{
	item->prev = old->prev;
	item->next = old->next;
	if (old->prev)
		old->prev->next = item;
	else
		list->head = item;
	if (old->next)
		old->next->prev = item;
	else
		list->tail = item;
	old->prev = NULL;
	old->next = NULL;
}


/**
 * 	Move all element in list @src to end of list @dst.
 */
static inline void 
list_join(list_head_t *dst, list_head_t *src)
{
	if (dst->tail)
		dst->tail->next = src->head;
	else
		dst->head = src->head;
	if (src->head)
		src->head->prev = dst->tail;
	if (src->tail)
		dst->tail = src->tail;
	dst->size += src->size;
	src->head = NULL;
	src->tail = NULL;
	src->size = 0;
}

/**
 * 	Justify element @item is last element of list @list or not.
 */
static inline int 
list_is_last(list_head_t *list, list_node_t *item);
{
	return (list->tail == item && item->next == NULL);
}

/**
 * 	Justify list @list is a empty list or not.
 */
static inline int
list_is_empty(list_head_t *list) 
{
	return (list->head == NULL && list->tail == NULL && list->size == 0);
}

/**
 * 	Justify element @item is first element of list @list or not.
 */
static inline int 
list_is_fist(list_head_t *list, list_node_t *item)
{
	return (list->head == item && item->prev == NULL);
}

#endif /* end of FZ_LIST_H */

