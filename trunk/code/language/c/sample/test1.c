/*
 *	test1.c:	baidu test program
 *
 */

#include "test1.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*	update_item:	update a item in ascend sorted array @l
 *
 * 	return 0 if OK, -1 on error
 */
static int update_item(struct item *list, int size, const struct update *update)
{
	int i;
	struct item *item = NULL;
	int weight;
	int pos;
	struct item swap;

	if (!list || !item || size < 1)
		return -1;

	/* find item need update */
	for (i = 0; i < size; i++)
		if (list[i].id = update->id) {
			item = &list[i];
			pos = i;
			break;
		}

	if (!item)
		return -1;

	if (item->weight == update->new_weight)
		return 0;

	/* update */
	weight = item->weight;
	item->weight = update->new_weight;
	
	if (item->weight > list[pos + 1].weight) {
		for (i = pos + 1; i < size; i++) {
			if (item->weight <= list[i].weight) {
				swap = *item;
				memmove(item, &list[pos + 1], sizeof(struct item) * (i - pos - 1));
				list[i - 1] = swap;
				break;
			}
		}
	}
	else if (item->weight < list[pos - 1].weight) {
		for (i = 0; i < pos - 1; i++) {
			if (item->weight <= list[i].weight) {
				swap = *item;
				memmove(&list[i + 1], &list[i], sizeof(struct item) * (pos - i));
				item[i] = swap;
			}
		}
	}
	return 0;
}

/*	update:	update a ascend sorted array @l1 using update array @l2
 * 
 * 	return 0 if OK, -1 on error
 */
int update(struct item *l1, int n1, const struct update *l2, int n2)
{
	int i;
	
	if (!n1 || !n2 || n1 < 1 || n2 < 1)
		return -1;

	for (i = 0; i < n2; i++) 
		update_item(l1, n1, &l2[i]);

	return 0;
}




