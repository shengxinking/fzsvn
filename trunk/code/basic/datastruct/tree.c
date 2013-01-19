/*
 * @file tree.c
 * @brief a common tree implement.
 *
 * @author Forrest.zhang
 */

#include <stdio.h>
#include <stdlib.h>

#include "tree.h"

tree_t *tree_alloc(treenode_cmp_func cmp_func, 
		treenode_find_func find_func,
		treenode_print_func print_func, 
		treenode_free_func free_func, 
		int magic)
{
	tree_t	*tree = NULL;

	tree = malloc(sizeof(tree_t));
	if (!tree)
		return NULL;

	tree->magic = magic;
	tree->size = 0;
	tree->root = NULL;
	tree->cmp_func = cmp_func;
	tree->find_func = find_func;
	tree->print_func = print_func;
	tree->free_func = free_func;

	return tree;
}

static int _treenode_free(treenode_t *node, treenode_free_func free_func)
{
	if (!node)
		return -1;

	if (node->firstchild)
		_treenode_free(node->firstchild, free_func);

	if (node->nextsibling)
		_treenode_free(node->nextsibling, free_func);

	if (free_func && node->data)
		free_func(node->data);

	free(node);

	return 0;
}

void tree_free(tree_t *tree)
{
	if (!tree)
		return;
	
	if (tree->root)
		_treenode_free(tree->root, tree->free_func);

	free(tree);	
}


int tree_add_end(tree_t *tree, treenode_t *father, void *data)
{
	treenode_t	*node = NULL;
	treenode_t	*child = NULL;

	if (!tree || !data)
		return -1;

	child = malloc(sizeof(treenode_t));
	if (!child)
		return -1;
	child->magic = tree->magic;
	child->father = NULL;
	child->firstchild = NULL;
	child->nextsibling = NULL;
	child->data = data;

	/* add @child as child of @father */
	if (father) {
		node = father->firstchild;
		if (!node)
			father->firstchild = child;
		else {
			while (node->nextsibling)
				node = node->nextsibling;

			node->nextsibling = child;
		}
		child->father = father;
	}
	else {
		if (tree->root) {
			free(child);
			return -1;
		}
		else {
			tree->root = child;
			child->father = child;
		}
	}

	tree->size++;

	return 0;
}

int tree_add_first(tree_t *tree, treenode_t *father, void *data)
{
	treenode_t	*node = NULL;
	treenode_t	*child = NULL;

	if (!tree || !data)
		return -1;

	child = malloc(sizeof(treenode_t));
	if (!child)
		return -1;
	child->magic = tree->magic;
	child->firstchild = NULL;
	child->nextsibling = NULL;
	child->father = NULL;
	child->data = data;

	if (father) {
		node = father->firstchild;
		if (!node)
			father->firstchild = child;
		else {
			child->nextsibling = node;
			father->firstchild = child;
		}
		child->father = father;
	}
	else {
		if (tree->root) {
			free(child);
			return -1;
		}
		else {
			tree->root = child;
			child->father = child;
		}
	}

	tree->size++;

	return 0;
}

treenode_t *_treenode_find(const tree_t *tree, 
			treenode_t *node, 
			const void *data)
{
	treenode_t *find = NULL;

	if (!tree || !node || !data)
		return NULL;

	if (tree->find_func(node->data, data) == 0)
		return node;

	if (node->firstchild) {
		find = _treenode_find(tree, node->firstchild, data);
		if (find)
			return find;
	}

	if (node->nextsibling) {
		find = _treenode_find(tree, node->nextsibling, data);
		if (find)
			return find;
	}
	
	return find;
}

treenode_t *tree_find(const tree_t *tree, const void *data)
{

	if (!tree || !data)
		return NULL;

	if (tree->find_func)
		return _treenode_find(tree, tree->root, data);
	else
		return NULL;
}

static int _treenode_del(tree_t *tree, treenode_t *node)
{
	if (!tree || !node)
		return 0;

	if (node->firstchild) {
		printf("free first child\n");
		_treenode_del(tree, node->firstchild);
	}

	if (node->nextsibling) {
		printf("free next sibling\n");
		_treenode_del(tree, node->nextsibling);
	}

	if (tree->free_func) {
		printf("free a node\n");
		tree->print_func(node->data);
		tree->free_func(node->data);
		tree->size--;
	}

	free(node);

	
	return 0;
}

void *tree_del(tree_t *tree, void *data)
{
	treenode_t *father = NULL;
	treenode_t *sibling = NULL;
	treenode_t *node = NULL;

	if (!tree || !data)
		return NULL;

	node = tree_find(tree, data);
	if (!node)
		return NULL;

	father = node->father;
	if (father) {
		if (father == node) {
			tree->root = NULL;
		}
		else {
			sibling = father->firstchild;
			if (sibling == node) {
				father->firstchild = node->nextsibling;
			}
			else {
				while(sibling->nextsibling &&
					sibling->nextsibling != node)
					sibling = sibling->nextsibling;
				if (sibling->nextsibling == NULL)
					return NULL;
				sibling->nextsibling = node->nextsibling;
			}
		}
	}
	else {
		if (tree->root != node)
			return NULL;

		tree->root = NULL;
	}
	node->father = NULL;
	node->nextsibling = NULL;
	_treenode_del(tree, node);

	return node;
}


static void _treenode_print(const tree_t *tree, 
			const treenode_t *node, int level)
{
	if (!node)
		return;

	if (tree->print_func) {
		printf("-----level is %d------\n", level);
		tree->print_func(node->data);
		printf("-----------------------\n");
	}

	if (node->firstchild) {
		_treenode_print(tree, node->firstchild, level + 1);
	}

	if (node->nextsibling) {
		_treenode_print(tree, node->nextsibling, level);
	}
}

void tree_print(const tree_t *tree)
{
	if (!tree)
		return;

	printf("tree: magic %x, size %d:\n", tree->magic, tree->size);
	if (tree->root)
		_treenode_print(tree, tree->root, 0);
	else
		printf("no root\n");
}


