/*
 * @file	tree.h
 * @brief	the common tree header file
 *
 * @author	Forrest.zhang
 */

#ifndef FZ_TREE_H
#define FZ_TREE_H

typedef struct treenode {
	int			magic;
	struct treenode		*firstchild;
	struct treenode		*nextsibling;
	struct treenode		*father;	/* add this for up search */
	void			*data;
} treenode_t;

typedef int (*treenode_cmp_func)(const void *p1, const void *p2);
typedef int (*treenode_find_func)(const void *data, const void *key);
typedef void (*treenode_print_func)(const void *data);
typedef void (*treenode_free_func)(void *data);

typedef struct tree {
	int			magic;
	treenode_t		*root;
	int			size;
	treenode_cmp_func	cmp_func;
	treenode_find_func	find_func;
	treenode_print_func	print_func;
	treenode_free_func	free_func;
} tree_t;

extern tree_t *tree_alloc(treenode_cmp_func cmp_func, 
			treenode_find_func find_func,
			treenode_print_func print_func,
			treenode_free_func free_func, 
			int magic);
extern void tree_free(tree_t *tree);

extern treenode_t *tree_find(const tree_t *tree, const void *data);

extern int tree_add_end(tree_t *tree, treenode_t *father, void *data);
extern int tree_add_first(tree_t *tree, treenode_t *father, void *data);
extern void *tree_del(tree_t *tree, void *data);

extern void tree_print(const tree_t *tree);

#endif /* end of FZ_TREE_H */

