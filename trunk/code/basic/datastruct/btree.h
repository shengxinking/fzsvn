/**
 *	@file	btree.h
 *
 *	@brief	binary-tree datastruct and APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2014-06-11
 */

#ifndef FZ_BTREE_H
#define FZ_BTREE_H

typedef	int	(*btree_cmp_func)(void *val1, void *val2);
typedef int	(*btree_list_func)(void *data, void *arg);
typedef	void	(*btree_free_func)(void *data);
typedef	void	(*btree_print_func)(void *data);

typedef struct btree_node {
	struct btree_node	*father;/* father */
	struct btree_node	*leftd;	/* left child */
	struct btree_node	*right;	/* right child */
	void			*data;	/* data in node */
} btree_node_t;

typedef struct btree {
	btree_node_t		*root;	/* root */
	u_int32_t		size;	/* elements number in tree */
	u_int32_t		high;	/* tree high */
	objpool_t		*pool;	/* tree_node pool */
	int			locked;	/* need lock or not */
	pthread_mutex_t		lock;	/* locked */
} btree_t;

extern btree_t *
btree_alloc(int locked);

extern int 
btree_free(btree_t *tree);

extern int 
btree_add(btree_t *tree, void *data);

extern int 
btree_del(btree_t *tree, void *data);

extern int 
btree_list(btree_t *tree, btree_list_func list_func, void *arg);

extern void 
btree_print(btree_t *tree, btree_print_func print_func);


#endif /* end of FZ_BTREE_H  */


