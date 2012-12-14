/*
 *	list_test.c	single list test program
 *
 * 	author		forrest.zhang
 */

#include "dlist.h"

#include <stdio.h>
#include <stdlib.h>

static void _usage(void)
{
	printf("list_test\n");
}

static int _parse_cmd(int argc, char **argv)
{
	return 0;
}

static int _init(void)
{
	return 0;
}

static int _release(void)
{
	return 0;
}

static int _int_cmp(const void *i, const void *j)
{
	if (!i && !j)
		return 0;
	else if (!i && j)
		return -1;
	else if (i && !j)
		return 1;
	else
		return (*(int *)i - *(int *)j);
}

static void _int_print(const void *i)
{
	if (i)
		printf("%d, ", *(int*)i);
}


int main(int argc, char **argv)
{
	dlist_t *list = NULL;
	dnode_t *node = NULL;
	int i = 2, j = 3, k = 4, m = 5, n = 6, l = 7;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init())
		return -1;

	list = dlist_create();

	printf("add %d to head\n", i);
	node = malloc(sizeof(dnode_t));
	if (!node)
		return -1;
	node->data = &i;
	node->next = NULL;
	dlist_add_prev(list, node, 0);
	dlist_print(list, _int_print);
	dlist_rprint(list, _int_print);

	printf("add %d to tail\n", j);
	node = malloc(sizeof(dnode_t));
	if (!node)
		return -1;
	node->data = &j;
	dlist_add_prev(list, node, 0);
	dlist_print(list, _int_print);
	dlist_rprint(list, _int_print);

	printf("add %d to next of 1\n", k);
	node = malloc(sizeof(dnode_t));
	if (!node)
		return -1;
	node->data = &k;
	dlist_add_next(list, node, 1);
	dlist_print(list, _int_print);
	dlist_rprint(list, _int_print);

	printf("add %d to prev of 2\n", l);
	node = malloc(sizeof(dnode_t));
	if (!node) 
		return -1;
	node->data = &l;
	dlist_add_prev(list, node, 2);
	dlist_print(list, _int_print);
	dlist_rprint(list, _int_print);

	printf("add %d to next of 2\n", m);
	node = malloc(sizeof(dnode_t));
	if (!node)
		return -1;
	node->data = &m;
	dlist_add_next(list, node, 2);
	dlist_print(list, _int_print);
	dlist_rprint(list, _int_print);

	printf("add %d to next of 4\n", n);
	node = malloc(sizeof(dnode_t));
	if (!node)
		return -1;
	node->data = &n;
	dlist_add_next(list, node, 4);
	dlist_print(list, _int_print);
	dlist_rprint(list, _int_print);

	printf("delete head of list\n");
	node = dlist_del_head(list);
	if (node)
		free(node);
	dlist_print(list, _int_print);
	dlist_rprint(list, _int_print);

	printf("delete tail of list\n");
	node = dlist_del_tail(list);
	if (node)
		free(node);
	dlist_print(list, _int_print);
	dlist_rprint(list, _int_print);

	printf("delete %d in list\n", k);
	node = dlist_del(list, &k, _int_cmp);
	if (node)
		free(node);
	dlist_print(list, _int_print);
	dlist_rprint(list, _int_print);

	printf("find %d in list\n", k);
	node = dlist_find(list, &k, _int_cmp);
	if (!node)
		printf("not find %d\n", k);
	else
		printf("find %d\n", k);

	printf("find %d in list reverse order\n", l);
	node = dlist_rfind(list, &l, _int_cmp);
	if (!node)
		printf("not find %d\n", l);
	else
		printf("find %d\n", l);

	while(list->size) {
		node = dlist_del_tail(list);
		dlist_print(list, _int_print);
		dlist_rprint(list, _int_print);
		free(node);
	}

	free(list);

	if (_release())
		return -1;
	
	return 0;
}

