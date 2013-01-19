/*
 *	list.c:	list implement using pointer
 *
 * 	author:	forrest.zhang
 */

#include "dlist.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/*	dlist_create	create a empty list using malloc
 *
 *	return list if OK, NULL if error(malloc error)
 */
dlist_t *dlist_create(void)
{
	dlist_t *list = NULL;
	list = malloc(sizeof(dlist_t));
	if (!list)
		return NULL;

	list->size = 0;
	list->head = list->tail = NULL;

	return list;
}

/*	dlist_empty	test if a list is empty
 *
 *	@list		list need test
 *
 * 	return 1 if @list is empty, 0 else.
 */
int dlist_empty(const dlist_t *list)
{
	if (!list || !list->size)
		return 1;
	else
		return 0;
}

/*	dlist_count	return how many elements in list
 *
 *	@list		list need count
 *
 *	return the count of @list if OK, -1 on error.
 */
int dlist_count(const dlist_t *list)
{
	if (!list)
		return -1;
	else
		return list->size;
}

/*	dlist_find	find which element's data is same as @data.
 *
 * 	@list		list need search
 * 	@data		value need find
 * 	@cmp		the compare function
 *
 * 	return pointer to element if OK, NULL if not found or error
 */
dnode_t *dlist_find(const dlist_t *list, const void *data, dlist_cmp_func cmp)
{
	dnode_t *node = NULL;

	/* verify parameter */
	if (!list || !data || !cmp || list->size < 1)
		return NULL;
	
	node = list->head;
	do {
		if (cmp(node->data, data) == 0) 
			return node;
		node = node->next;
	} while (node != list->head);
		
	return NULL;
}

/*	dlist_rfind	find which element's data is same as @data from tail start.
 *
 * 	@list		list need search
 * 	@data		value need find
 * 	@cmp		the compare function
 *
 * 	return pointer to element if OK, NULL if not found or error
 */
dnode_t *dlist_rfind(const dlist_t *list, const void *data, dlist_cmp_func cmp)
{
	dnode_t *node = NULL;

	/* verify parameter */
	if (!list || !data || !cmp || list->size < 1)
		return NULL;
	
	node = list->tail;
	do {
		if (cmp(node->data, data) == 0) 
			return node;
		node = node->prev;
	} while (node != list->tail);
		
	return NULL;
}

/*	dlist_get	get the list[pos] element
 *
 *	@list		list
 *	@pos		the position of element need return
 *
 *	return the pointer to element if OK, NULL on error
 */
dnode_t *dlist_get(const dlist_t *list, int pos)
{
	int i;
	dnode_t	*node;

	if (!list || pos < 0 || pos >= list->size)
		return NULL;

	node = list->head;
	for (i = 0; i < pos; i++)
		node = node->next;

	return node;
}

/*	dlist_add_next	add a node after @list[pos]
 *
 *	@list		list
 *	@node		element need add
 *	@pos		postion need add
 *
 *	return 0 if OK, -1 on error
 */
int dlist_add_next(dlist_t *list, dnode_t *node, int pos)
{
	dnode_t *n = NULL;

	if (!node || !list)
		return -1;

	if (list->size == 0) {
		node->next = node->prev = node;
		list->head = list->tail = node;
		list->size++;
		return 0;
	}

	n = dlist_get(list, pos);
	if (!n)
		return -1;

	node->next = n->next;
	node->prev = n;
	n->next = n->next->prev = node;

	list->tail = list->head->prev;

	list->size++;

	return 0;
}

/*	dlist_add_prev	add a node before @list[pos]
 *
 *	@list		list
 *	@node		element need add
 *	@pos		postion need > 0
 *
 *	return 0 if OK, -1 on error
 */
int dlist_add_prev(dlist_t *list, dnode_t *node, int pos)
{
	dnode_t *n = NULL;

	if (!list || !node)
		return -1;

	if (list->size == 0) {
		node->next = node->prev = node;
		list->head = list->tail = node;
		list->size++;
		return 0;
	}

	n = dlist_get(list, pos);
	if (!n)
		return -1;

	node->prev = n->prev;
	node->next = n;
	n->prev = node->prev->next = node;

	list->head = list->tail->next;

	list->size++;

	return 0;
}

/*	dlist_del	delete element @list[pos]
 *
 *	@list		list
 *	@pos		index of list element
 *
 *	return pointer to element deleted if OK, -1 on error
 */
dnode_t *dlist_del(dlist_t *list, const void *data, dlist_cmp_func cmp)
{
	dnode_t *node = NULL;

	node = dlist_find(list, data, cmp);
	if (!node)
		return NULL;
	
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->prev = node->next = NULL;

	if (node == list->head)
		list->head = list->tail->next;
	else if (node == list->tail)
		list->tail = list->head->prev;

	list->size--;

	return node;
}

/*	dlist_del_head	delete head element of @list
 *
 *	@list		list
 *
 *	return pointer to element deleted if OK, -1 on error
 */
dnode_t *dlist_del_head(dlist_t *list)
{
	dnode_t *node = NULL;

	if (!list || list->size < 1)
		return NULL;

	node = list->head;

	if (list->size == 1) {
		node->prev = node->next = NULL;
		list->head = list->tail = NULL;
		list->size --;
		return node;
	}

	node->next->prev = node->prev;
	node->prev->next = node->next;
	list->head = list->tail->next;
	node->next = node->prev = NULL;

	list->size--;

	return node;
}

/*	list_del_tail	delete tail element of @list
 *
 *	@list		list
 *
 *	return pointer to element deleted if OK, -1 on error
 */
dnode_t *dlist_del_tail(dlist_t *list)
{
	dnode_t *node = NULL;
	
	if (!list || list->size < 1)
		return NULL;

	node = list->tail;

	if (list->size == 1) {
		node->next = node->prev = NULL;
		list->head = list->tail = NULL;
		list->size --;
		return node;
	}

	node->next->prev = node->prev;
	node->prev->next = node->next;
	list->tail = list->head->prev;

	node->next = node->prev = NULL;

	list->size--;

	return node;
}

/*	list_print	print all element in @list
 *
 *	@list		list
 *	@print		print function of each element
 *
 *	no return
 */
void dlist_print(const dlist_t *list, dlist_print_func print)
{
	dnode_t *node = NULL;

	if (!list || !print)
		return;

	if (list->size == 0) {
		printf("empty list\n");
		return;
	}


	printf("%d element in list:\n", list->size);
	node = list->head;
	do {
		print(node->data);
		node = node->next;
	} while(node != list->head);

	printf("\n\n");
}

/*	list_rprint	print all element in @list in reverse order
 *
 *	@list		list
 *	@print		print function of each element
 *
 *	no return
 */
void dlist_rprint(const dlist_t *list, dlist_print_func print)
{
	dnode_t *node = NULL;

	if (!list || !print)
		return;

	if (list->size == 0) {
		printf("empty list\n");
		return;
	}

	printf("%d element in list(reverse order):\n", list->size);
	node = list->tail;
	do {
		print(node->data);
		node = node->prev;
	} while(node != list->tail);
	printf("\n\n");
}


