/*
 * @file        hash_test.c
 * @brief       test hash.c functions
 *
 * @author      Forrest.zhang
 */

#include <stdio.h>
#include <unistd.h>


#include "hash.h"

typedef struct person {
	char       name[8];
	int        age;
	u_int32_t  id;
} person_t;

static int _person_cmp(const void *data, const void *arg)
{
	person_t *p1;
	int id;

	if (!data || !arg)
		return -1;
	
	p1 = (person_t*)data;
	id = *((int *)arg);

	if (p1->id == id)
		return 0;

	return 1;
}

static void _person_print(const void *data)
{
	person_t *p1;

	if (!data) {
		printf("NULL\n");
		return;
	}

	p1 = (person_t *)data;
	printf("name %s, age %d, id %u\n", p1->name, p1->age, p1->id);
}

int main(void)
{
	hash_t *hash;
	person_t p1 = {"FZ", 29, 1};
	person_t p2 = {"SB", 31, 8};
	person_t p3 = {"ZZ", 39, 15};
	person_t *p;
	

	hash = hash_alloc(3, NULL);
	if (!hash) {
		printf("alloc hash error\n");
		return -1;
	}
	
	hash_put(hash, p1.id, &p1);

	hash_put(hash, p2.id, &p2);

	hash_put(hash, p3.id, &p3);

	p = hash_get(hash, 7, NULL);
	if (p)
		_person_print(p);
	else
		printf("not found %d\n", 7);

	
	hash_remove(hash, p1.id, NULL);
	hash_remove(hash, p2.id, NULL);
	hash_remove(hash, p3.id, NULL);

	hash_free(hash);

	return 0;
}
