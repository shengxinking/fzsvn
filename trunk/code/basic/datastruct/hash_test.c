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
	u_int32_t  id;
	char       name[8];
	int        age;
} person_t;

static int 
_person_cmp(const void *val1, const void *val2)
{
	u_int32_t *p1;
	u_int32_t *p2;

	if (!val1 || !val2)
		return -1;
	
	p1 = (u_int32_t *)val1;
	p2 = (u_int32_t *)val2;

	return (p1 - p2);
}

static void 
_person_print(const void *data)
{
	person_t *p1;

	if (!data) {
		printf("NULL\n");
		return;
	}

	p1 = (person_t *)data;
	printf("id %u, name %s, age %d\n", p1->id, p1->name, p1->age);
}

int 
main(void)
{
	hash_t *hash;
	person_t p1 = {1,  "FZ", 29};
	person_t p2 = {8,  "SB", 31};
	person_t p3 = {39, "ZZ", 39};
	person_t *p;
	
	hash = hash_alloc(1, _person_cmp, 1);
	if (!hash) {
		printf("alloc hash error\n");
		return -1;
	}
	
	if (hash_insert(hash, p1.id, &p1)) {
		printf("insert hash failed\n");
		hash_free(hash, NULL);
		return -1;
	}

	hash_print(hash, _person_print);

	if (hash_insert(hash, p2.id, &p2)) {
		printf("insert hash failed\n");
		hash_free(hash, NULL);
		return -1;
	}

	hash_print(hash, _person_print);

	if (hash_insert(hash, p3.id, &p3)) {
		printf("insert hash failed\n");
		hash_free(hash, NULL);
		return -1;
	}

	hash_print(hash, _person_print);

	p = hash_search(hash, 39, &p3);
	if (p)
		_person_print(p);
	else
		printf("not found %d\n", 39);

	hash_print(hash, _person_print);
	
	hash_delete(hash, p2.id, &p2);
	hash_delete(hash, p1.id, &p1);
	hash_delete(hash, p3.id, &p3);

	hash_print(hash, _person_print);

	hash_free(hash, NULL);

	return 0;
}
