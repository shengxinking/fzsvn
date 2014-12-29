/*
 * @file	tree.h
 * @brief	the common tree header file
 *
 * @author	Forrest.zhang
 */

#ifndef FZ_TREE_H
#define FZ_TREE_H

typedef int (*tree_find_func)(const void *p1, const void *p2);
typedef int (*tree_list_func)(void *data, void *arg);
typedef void (*tree_print_func)(const void *data);
typedef void (*tree_free_func)(void *data);

typedef struct tree_node {
	struct tree_node	*first_child;	/* first child */
	struct tree_node	*next_sibling;	/* next sibling */
	struct tree_node	*father;	/* father */
	void			*data;		/* data in tree */
} tree_node_t;

typedef struct tree {
	tree_node_t		*root;		/* root of tree */
	int			size;		/* element number in tree */
	int			high;		/* the high of tree */
	treenode_cmp_func	cmp_func;	/* compare function for find */
	objpool_t		*pool;		/* object pool */
	int			need_lock;	/* locked or not */
	pthread_mutex_t		lock;		/* lock for thread-safe */
} tree_t;

extern tree_t 
*tree_alloc(tree_node_cmp_func cmp_func)

extern void 
tree_free(tree_t *t, tree_free_func free_func);

extern int 
tree_add_first(tree_t *t, const void *father, void *data);

extern int 
tree_add_tail(tree_t *tree, const void *father, void *data);

extern int 
tree_add_before(tree_t *tree, const void *father, const void *sibling void *data);

extern int 
tree_add_after(tree_t *tree, const void *father, const void *sibling void *data);

extern void * 
tree_find(const tree_t *tree, const void *data);

extern int 
tree_del(tree_t *tree, const void *data);

extern int 
tree_list(tree_t *tree, tree_list_func list_func, void *arg);

extern int 
tree_print(tree_t *tree);

#endif /* end of FZ_TREE_H */

