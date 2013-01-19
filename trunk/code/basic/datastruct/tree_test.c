/*
 * @file tree_test.c
 * @brief the tree.c test program
 *
 * @author Forrest.zhang
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "tree.h"

#define NAMELEN		20

typedef struct person {
	char	name[NAMELEN];
	int	age;
} person_t;

static void person_print(const void *data)
{
	person_t *person = NULL;
	if (data) {
		person = (person_t*)data;
		printf("name %s age %d\n", person->name, person->age);
	}
}

static int person_find(const void *p1, const void *p2)
{
	person_t *person1, *person2;

	if (!p1 || !p2)
		return -1;
	
	person1 = (person_t*)p1;
	person2 = (person_t*)p2;

	if (strncmp(person1->name, person2->name, NAMELEN - 1) == 0 &&
		person1->age == person2->age)
		return 0;

	return -1;
}

static void person_free(void *p1)
{
	if (p1) {
		free(p1);
	}
}

static void _usage(void)
{
}

static int _parse_cmd(int argc, char **argv)
{
	return 0;
}

static int _init(void)
{
	return 0;
}

static void _release(void)
{
}

int main(int argc, char **argv)
{
	tree_t *tree = NULL;
	treenode_t *node = NULL;
	person_t *person = NULL;

	if (_parse_cmd(argc, argv)) {
		_usage();
		return -1;
	}

	if (_init())
		return -1;

	tree = tree_alloc(NULL, person_find, person_print, 
			person_free, 0x1234);

	person = malloc(sizeof(person_t));
	if (!person)
		goto out_free;
	
	strncpy(person->name, "fz", NAMELEN - 1);
	person->age = 33;
	tree_add_end(tree, NULL, person);

	node = tree_find(tree, person);
	if (node) {
		printf("find (%s:%d) in tree\n", person->name, person->age);
		person = malloc(sizeof(person_t));
		if (!person)
			goto out_free;
		strncpy(person->name, "forrest", NAMELEN -1);
		person->age = 11;

		tree_add_first(tree, node, person);
	
		person = malloc(sizeof(person_t));
		if (!person)
			goto out_free;
		strncpy(person->name, "bug", NAMELEN -1);
		person->age = 13;

		tree_add_end(tree, node, person);
	}

	tree_print(tree);

	person = malloc(sizeof(person_t));
	if (!person)
		goto out_free;
	strncpy(person->name, "fz", NAMELEN -1);
	person->age = 33;
	node = tree_find(tree, person);
	if (node) {
		printf("find (%s:%d) in tree\n", person->name, person->age);
	}
	tree_del(tree, person);
	free(person);

	tree_print(tree);

out_free:
	tree_free(tree);

	_release();
	
	return 0;
}


