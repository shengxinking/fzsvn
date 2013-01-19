/*
 *	list.c:	list implement using pointer
 *
 * 	author:	forrest.zhang
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "list.h"

typedef struct listnode {
	void		*data;
	struct listnode	*next;	
} listnode_t;

list_t *list_alloc(list_cmp_func cmp_func, 
		list_find_func find_func,
		list_print_func print_func,
		list_free_func free_func)
{
	list_t *list = NULL;

	list = malloc(sizeof(list_t));
	if (!list)
		return NULL;

	list->size = 0;
	list->pos = 0;
	list->front = NULL;
	list->rear = NULL;
	list->cmp_func = cmp_func;
	list->find_func = find_func;
	list->print_func = print_func;
	list->free_func = free_func;

	return list;
}

void list_free(list_t *list)
{
	listnode_t *node = NULL;
	listnode_t *next = NULL;

	if (!list)
		return;

	if (list->size > 0) {
		node = list->front;
		while (node) {
			next = node->next;
			if (list->free_func)
				list->free_func(node->data);
			free(node);
			node = next;
		}
	}

	free(list);
}

int list_empty(const list_t* list)
{
	if (list)
		return (list->size == 0);
	else
		return 1;
}

int list_count(const list_t *list)
{
	if (list)
		return list->size;
	else
		return -1;
}

void *list_find(list_t *list, void *data)
{
	listnode_t *node = NULL;
	int i;

	/* verify parameter */
	if (!list || !data || !list->find_func)
		return NULL;
	
	i = 0;
	node = list->front;
	while (node) {
		if (list->find_func(node->data, data) == 0) {
			list->pos = i;
			return node->data;
		}
		node = node->next;
		i++;
	}
	
	return NULL;
}

void *list_get(list_t *list, int pos)
{
	listnode_t *node = NULL;
	int i;

	if (!list || pos < 0 || pos >= list->size)
		return NULL;

	i = 0;
	node = list->front;
	while (node && i < pos) {
		node = node->next;
		i++;
	}

	list->pos = pos;

	return node->data;
}

int list_iterator(list_t *list, list_iterator_func iterator_func)
{
	int i;
	listnode_t *node = NULL;

	if (!list || !iterator_func)
		return -1;

	if (list->size = 0)
		return 0;

	i = 0;
	node = list->front;
	while (node) {
		list->pos = i;
		if (iterator_func(node->data, i))
			break;
		/* only increment i when current pos is less than @i */
		if (list->pos > i)
			i++;
		node = node->next;
	}
	
	return 0;
}

int list_add_next(list_t *list, int pos, void *data)
{
	listnode_t *node = NULL;
	listnode_t *newnode = NULL;
	int index = 0;

	if (!list || !data || pos < 0 || pos > list->size)
		return -1;
	
	if (pos == 0 && list->size == 0)
		return list_add_head(list, data);

	if (pos == list->size)
		return list_add_tail(list, data);

	node = list->front;
	while(node && index < pos)
		node = node->next;
	if (!node)
		return -1;

	newnode = malloc(sizeof(listnode_t));
	if (!newnode)
		return -1;

	newnode->data = data;
	newnode->next = node->next;
	node->next = newnode;
	
	list->pos = pos + 1;
	list->size++;

	return 0;
}

int list_add_prev(list_t *list, int pos, void *data)
{
	listnode_t *node = NULL;
	listnode_t *newnode = NULL;
	int index = 0;

	if (!list || !data || pos < 0 || pos > list->size)
		return -1;
	
	if (pos == 0)
		return list_add_head(list, data);

	if (pos == list->size)
		return list_add_tail(list, data);

	node = list->front;
	while(node && index < pos - 1)
		node = node->next;
	if (!node)
		return -1;

	newnode = malloc(sizeof(listnode_t));
	if (!newnode)
		return -1;

	newnode->data = data;
	newnode->next = node->next;
	node->next = newnode;

	list->pos = pos - 1;
	list->size++;

	return 0;
}

int list_add_tail(list_t *list, void *data)
{
	listnode_t *node = NULL;
	listnode_t *newnode = NULL;

	if (!list || !data)
		return -1;

	newnode = malloc(sizeof(listnode_t));
	if (!newnode)
		return -1;
	newnode->data = data;
	newnode->next = NULL;

	node = list->rear;
	if (!node) {
		list->front = list->rear = newnode;
	}
	else {
		node->next = newnode;
		list->rear = newnode;
	}

	list->size++;
	list->pos = list->size;

	return 0;
}

int list_add_head(list_t *list, void *data)
{
	listnode_t *node = NULL;
	listnode_t *newnode = NULL;

	if (!list || !data)
		return -1;
	
	newnode = malloc(sizeof(listnode_t));
	if (!newnode)
		return -1;
	newnode->data = data;
	newnode->next = NULL;

	node = list->front;
	if (!node) {
		list->front = list->rear = newnode;
	}
	else {
		newnode->next = list->front;
		list->front = newnode;
	}

	list->pos = 0;
	list->size++;

	return 0;
}

int list_add_asc(list_t *list, void *data)
{
	return 0;
}

int list_add_dsc(list_t *list, void *data)
{
	return 0;
}

int list_del(list_t *list, int pos)
{
	listnode_t *prev = NULL;
	listnode_t *node = NULL;
	int index = 0;

	if (!list || pos < 0 || pos >= list->size)
		return -1;

	if (pos == 0)
		return list_del_head(list);

	if (pos == list->size - 1)
		return list_del_tail(list);

	prev = list->front;
	while (prev && index < pos - 1)
		prev = prev->next;

	node = prev->next;
	prev->next = prev->next->next;

	if (list->free_func)
		list->free_func(node->data);

	free(node);

	list->pos = pos;
	list->size--;

	return 0;
}

int list_del_head(list_t *list)
{
	listnode_t *node = NULL;

	if (!list || list->size < 1)
		return -1;
	
	node = list->front;
	list->front = node->next;
	if (list->size == 1)
		list->rear = NULL;

	if (list->free_func)
		list->free_func(node->data);

	free(node);

	list->pos = 0;
	list->size--;

	return 0;
}

int list_del_tail(list_t *list)
{
	listnode_t *prev = NULL;
	listnode_t *node = NULL;

	if (!list || list->size < 1)
		return -1;

	prev = list->front;
	node = list->rear;
	while (prev->next != node && prev->next)
		prev = prev->next;

	if (prev->next == NULL) {
		list->front = NULL;
		list->rear = NULL;
	}
	else {
		list->rear = prev;
		prev->next = NULL;
	}

	if (list->free_func)
		list->free_func(node->data);

	free(node);

	list->size--;

	return 0;
}

list_t *list_sort_asc(const list_t *list)
{
	return NULL;
}

list_t *list_sort_dsc(const list_t *list)
{
	return NULL;
}

void list_print(const list_t *list)
{
	listnode_t *node = NULL;

	if (!list || !list->print_func)
		return;

	printf("list: size %d\n", list->size);
	
	if (list->size) {
		node = list->front;
		while (node) {
			if (list->print_func)
				list->print_func(node->data);
			node = node->next;
		}
	}	
}


