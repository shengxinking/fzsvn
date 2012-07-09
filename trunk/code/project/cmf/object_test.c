/*
 * @file	object_test.c
 * @brief	the test program for object.c
 *
 * @author	Forrest.zhang
 */

#include <unistd.h>
#include <stdlib.h>

#include "object.h"

typedef struct person {
	char	name[CLI_NAMELEN + 1];
	int	age;
	int	status;
} person_t;

int main(void)
{
	void *obj = NULL;
	void *attr = NULL;

	cli_option_t opt [] = {
		{1, "Enable"},
		{0, "Disable"},
	};
	
	obj = cli_object_add("system", "person", "person infomation", 
				  CLI_OBJ_UNI, 1, CLI_OBJ_GRP_RDWR,  0, 
				  sizeof(person_t));
	if (!obj) {
		return -1;
	}

	attr = cli_attr_add(obj, "name", "name of person", CLI_DTYPE_STR, 
				 CLI_NAMELEN + 1, CLI_OFFSET(person_t, name));

	
	attr = cli_attr_add(obj, "age", "age of person", CLI_DTYPE_INT,
				 sizeof(int), CLI_OFFSET(person_t, age));

	attr = cli_attr_add(obj, "status", "The status of person", 
				 CLI_DTYPE_INT, sizeof(int), 
				 CLI_OFFSET(person_t, status));

	cli_object_set_option(attr, opt, sizeof(opt)/sizeof(cli_option_t));

	cli_object_print(NULL);

	cli_object_free(NULL);

	return 0;
}

