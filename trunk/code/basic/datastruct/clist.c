/*
 *	clist.c:	array implement linked list
 *
 *	author:		forrest.zhang
 */

#include "clist.h"

clist_t *clist_create(size_t objsize, size_t maxobjs)
{
	clist_t *list = NULL;
	size_t size = 0;

	size = objsize + sizeof(cnode_t);
	list = malloc(objsize * maxobjs + sizeof(clist_t));
	if (!list)
		return list;
	memset(list, 0, objsize * maxobjs + sizeof(clist_t));

	list->objsize = objsize;
	list->size = maxobjsize;
}


