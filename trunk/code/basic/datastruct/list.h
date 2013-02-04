/*
 *	list.h:		the head file of list data struct, it using pointer 
 *			implement index
 *
 * 	author:		forrest.zhang
 */

#ifndef FZ_LIST_H
#define FZ_LIST_H

#include <sys/types.h>

#define	LIST_HEAD_INIT(name) { &(name), &(name) }
#define	LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

typedef struct list_head {
	struct list_head	*next;	/* the next object */
	struct list_head	*prev;	/* the previous object */
} list_head_t;

typedef struct hlist_node {
	struct hlist_node	*next;	/* the next object */
	struct hlist_node	**prev;	/* the previous object address */
} hlist_node_t;

typedef struct hlist_head {
	struct hlist_node	*first;	/* the first object in hash list */
} hlist_head_t;


static inline void 
list_add(list_head_t *list, list_head_t *item);


static inline void 
list_add_tail(list_head_t *head, list_head_t *item);

static inline void 
list_del(list_head_t *item);

static inline void 
list_replace(list_head_t *old, list_head_t *new);

static inline void 
list_join(list_head_t *list, list_head_t *new);

static inline int 
list_is_last(list_head_t *list, list_head_t *item);

static inline int
list_is_empty(list_head_t *list);

static inline int 
list_is_fist(list_head_t *list, list_head_t *new);


#endif /* end of FZ_LIST_H */

