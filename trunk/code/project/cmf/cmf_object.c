/**
 *	@file	object.c
 *	@brief	object implement, a object is an abstract of a 
 *		struct or a member of struct
 *
 *	@author	Forrest.zhang
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>

#include "cli_debug.h"
#include "cli_object.h"
#include "cli_error.h"


static void	*cli_objmem = NULL;
static void	*cli_optmem = NULL;
static u_int16_t cli_objid = 0;
static u_int16_t cli_optid = 0;



/**
 *	Alloc a new cli_object_t object.
 *
 *	Return object pointer if success, NULL on error.
 */
cli_object_t * 
_cli_alloc_object(const char *name, u_int8_t type, u_int16_t off, u_int16_t size)
{
	cli_object_t *obj = NULL;
	u_int16_t id;

	if (!name)
		return NULL;
	
	if (!cli_objmem) {
		CLI_ERR("need call cli_init_objtree\n");
		return NULL;
	}
	
	if (cli_objid >= CLI_OBJECT_MAX)
		return NULL;

	obj = CLI_OBJPTR(cli_objid);

	str_cpy(obj->name, name, CLI_NAMELEN);	
	obj->type = type;
	obj->offset = offset;
	obj->size = size;
	obj->magic = CLI_OBJ_MAGIC;

	cli_objtree.freeobjid++;

	return obj;
}


/**
 *	Alloc options for object.
 *
 *	Return option pointer if success, NULL on error.
 */
cli_option_t *
_cli_alloc_options(int nopt)
{
	cli_option_t *opt = NULL;

	if (!cli_obtmem) {
		CLI_ERR("need call cli_init_objtree\n");
		return NULL;
	}

	if ((cli_optid + nopt) > CLI_OPTION_MAX) 
		return NULL;

	opt = CLI_OPTPTR(cli_optmem, optid);
	cli_optid += nopt;

	return opt;
}


/**
 *	Add a object @obj to object @parent's as it's child, 
 *	it's last children
 *
 *	No return.
 */
static void 
_cli_add_tail(cli_object_t *parent, cli_object_t *obj)
{
	cli_object_t *brother;

	assert(parent);
	assert(obj);

	brother = parent->childs;
	if (!brother) {
		assert(parent->nchilds == 0);
		parent->childs = obj;
	}
	else {
		while (brother->sibling)
			brother = brother->sibling;
		
		brother->sibling = obj;
	}

	obj->parent = parent;
	parent->nchilds++;
}


/**
 *	Split path string @path to object name, the path format is like:
 *	"system.global", the dot is the seperator of each object name.
 *	The path string can only contain [ALPHA][NUM][-.] chars, '.' is the
 *	seperator char. Each object name not exceed CLI_NAMEMAX;
 *
 *	Return the object name array if success, NULL on error.
 */
static char ** 
_cli_split_path(const char *path, int delim)
{
	int count = 1;
	int i;
	int len = 0;
	const char *ptr;
	const char *begin;
	char **paths;

	assert(path);

	/* check path is valid */
	ptr = path;
	while (*ptr) {
		if (isalnum(*ptr) || *ptr == '-' || *ptr == delim)
			ptr++;
		else {
			g_cli_errno = CLI_INVALID_PARAM;
			return NULL;
		}
	}

	ptr = strchr(path, delim);
	/* the first char is not delim, it's error */
	if (ptr == path) {
		g_cli_errno = CLI_INVALID_PARAM;
		return NULL;
	}
       
	/* how many node in path */
	begin = path;
	while (ptr) {
		if (ptr[1] != 0) {
			
			/* include '..', it's error */
			if (ptr[1] == '.') {
				g_cli_errno = CLI_INVALID_PARAM;
				return NULL;
			}
			
			/* object name is too long */
			len = ptr - begin;
			if (len > CLI_NAMELEN) {
				g_cli_errno = CLI_NAME_TOOLONG;
				return NULL;
			}

			count++;
			begin = ptr + 1;
			ptr = strchr(ptr + 1, '.');
		}
		else
			break;
	}

	/* malloc object name string array */
	paths = malloc((count + 1) * sizeof(char *));
	if (!paths) {
		g_cli_errno = CLI_INVALID_PARAM;
		return NULL;
	}
	memset(paths, 0, sizeof(char *) * (count + 1));

	begin = path;
	ptr = strchr(path, '.');
	for (i = 0; i < count; i++) {
		paths[i] = malloc(CLI_NAMELEN + 1);
		if (!paths[i]) {
			g_cli_errno = CLI_MALLOC_FAILED;
			_cli_object_free_pathlist(paths);
			return NULL;
		}
		memset(paths[i], 0, CLI_NAMELEN + 1);
		
		if (ptr) {
			len = ptr - begin;
			str_cpy(paths[i], begin, len);

			begin = ptr + 1;
			ptr = strchr(begin, delim);
		}
		else {
			str_cpy(paths[i], begin, CLI_NAMELEN);
			break;
		}
	}

	return paths;
}


/**
 *	Find the child in cli_object_t @obj's children.
 *
 * 	Return the object of if success, NULL on error.
 */
static cli_object_t *  
_cli_find_child(cli_object_t *obj, const char *name)
{
	cli_object_t *child = NULL;

	if (!obj || !name)
		return -1;

	child = obj->childs;
	if (!child)
		return NULL;

	do {
		/* find path */
		if (strncmp(child->name, name, CLI_NAMELEN) == 0)
			break;
		
		child = child->sibling;
	} while (child);

	return child;
}


/**
 *	Find the last object of @path and return it, if create is non-zero,
 *	it'll create the object if need.
 *
 *	Return the object if success, NULL on error or not founded. 
 */
static void *
_cli_find_path(const char *path, int create)
{
	cli_object_t *root = &cli_rootobj;
	cli_object_t *parent = NULL, *obj = NULL;
	char **paths = NULL;
	int i = 0;

	paths = _cli_split_path(path, '.');
	if (!paths)
		return NULL;

	parent = root;
	while (paths[i]) {
		obj = _cli_find_child(parent, paths[i]);
		if (!obj) {
			if (!create) {
				return NULL;
			}

			obj = _cli_alloc_object(paths[i], CLI_OBJ_PATH, 0, 0);
			if (!obj) {
				return NULL;
			}

			_cli_add_tail(parent, obj);
		}

		parent = obj;
		i++;
	}

	_cli_free_pathlist(paths);

	return obj;
}


/**
 *	Print leading space char.
 *
 *	No return.
 */
static void 
_cli_print_level(int level, char c)
{
	int i;

	for (i = 0; i < level; i++)
		printf("%c%c%c%c", c, c, c, c);
}


/**
 *	Print subtree objects.
 *
 *	No return.
 */
static void 
_cli_print_tree(cli_object_t *root, int level)
{
	cli_object_t *obj = NULL;
	cli_object_t *sibling = NULL;
	int i;

	_cli_object_print_level(level, '-');
	printf("(%p) magic %8x, name %s, type %2x, group %2x\n",            
	       root, root->magic, root->name, root->type, root->group);
	
	_cli_object_print_level(level, ' ');
	printf("perm %2x, size %d, offset %d, dtype %d\n",
	       root->perm, root->size, root->offset, root->dtype);

	_cli_object_print_level(level, ' ');
	printf("nchilds %d, childs %p, sibling %p, father %p, ops %p\n",
	       root->nchilds, root->childs, root->sibling, 
	       root->parent, root->ops);

	_cli_object_print_level(level, ' ');
	printf("opts %p, nopts %d:\n", root->opts, root->nopts);
	
	for (i = 0; i < root->nopts; i++) {
		_cli_object_print_level(level, ' ');
		printf("val %d, name %s\n", root->opts[i].val, 
		       root->opts[i].name);
	}
	printf("\n");

	obj = root->childs;
	while (obj) {
		sibling = obj->sibling;		
		_cli_object_print(obj, level + 1);
		obj = sibling;
	}
}


int 
cli_init_objtree(void)
{
	cli_object_t *root = NULL;

	/* alloc memory for object */
	if (cli_objmem) {
		free(cli_objmem);
	}
	cli_objmem = malloc(sizeof(cli_object_t) * CLI_CHILD_MAX);
	if (!cli_objmem) {
		CLI_ERR("malloc memory failed\n");
		return -1;
	}
	memset(cli_objmem, 0, sizeof(cli_object_t) * CLI_CHILD_MAX);

	/* alloc memory for option */
	if (cli_optmem) {
		free(cli_optmem);
	}
	cli_optmem = malloc(sizeof(cli_option_t) * CLI_OPTION_MAX);
	if (!cli_optmem) {
		CLI_ERR("malloc memory failed\n");
		return -1;
	}
	memset(cli_optmem, 0, sizeof(cli_option_t) * CLI_OPTION_MAX);

	root = CLI_OBJPTR(cli_objmem, 0);
	root->magic = CLI_OBJ_MAGIC;
	root->type = CLI_OBJ_PATH;

	cli_objid = 1;

	return 0;
}


void 
cli_free_obtree(void)
{
	if (cli_objmem) {
		free(cli_objmem);
		cli_objmem = NULL;
	}

	if (cli_optmem) {
		free(cli_optmem);
		cli_optmem = NULL;
	}

	cli_optid = 0;
	cli_objid = 0;
}



void * 
cli_add_object(const char *path, const char *name, u_int8_t type, 
	       u_int16_t offset, u_int16_t size)
{
	cli_object_t *obj = NULL, *parent = NULL;
	
	if (!path || !name || size < 1)
		return NULL;
	
	parent = _cli_find_path(path, 1);
	if (!parent) {
		return NULL;
	}

	obj = parent->childs;
	while (obj) {
		if (strncmp(name, obj->name, sizeof(obj->name)) == 0) {
			CLI_DBG("object %s is exist\n", name);
			errno = EEXIST;
			return NULL;
		}

		obj = obj->sibling;
	}

	obj = _cli_alloc_object(name, type, offset, size);
	if (!obj) {
		return NULL;
	}

        _cli_object_add_tail(parent, obj);

	return obj;
}




void * 
cli_add_attribute(void *obj, const char *name, u_int16_t off, u_int16_t size)
{
	cli_object_t *obj1 = obj;
	cli_object_t *attr = NULL;

	if (!obj1 || !name || size < 1)
		return NULL;

	assert(obj1->magic == CLI_OBJ_MAGIC);

	attr = _cli_alloc_object(name, size);
	if (!attr) {
		return NULL;
	}	
	str_cpy(attr->name, name, CLI_NAMELEN);

	attr->dtype = type;
	attr->offset = offset;

	_cli_object_add_tail(obj, attr);

	return attr;
}


int 
cli_set_objcomment(void *obj, const char *comment)
{
	cli_object_t *obj1 = (cli_object_t *)obj;

	if (!obj1 || !comment)
		return -1;

	str_cpy(obj1->comment, comment, CLI_COMMLEN);

	return 0;
}


int 
cli_set_objflags(void *obj, u_int8_t flags)
{
	cli_object_t *obj1 = NULL;

	if (!obj)
		return -1;

	obj1 = (cli_object_t *)obj;
	assert(obj1->magic == CLI_OBJ_MAGIC);

	obj1->flags = flags;
}


int 
cli_set_objfunc(void *obj, cli_objfunc_t funcs)
{
	cli_object_t *obj1;

	if (!obj || !funcs)
		return -1;

	obj1 = (cli_object_t *)obj;
	assert(obj1->magic == CLI_OBJ_MAGIC);

	obj1->funcs = funcs;

	return 0;
} 


int 
cli_set_objperm(void *obj, u_int8_t group, u_int8_t perm)
{
	cli_object_t *obj1 = NULL;

	if (!obj)
		return -1;

	obj1 = (cli_object_t *)obj;
	assert(obj1->magic == CLI_OBJ_MAGIC);

	obj1->group = group;
	obj1->perm = perm;
	
	return 0;
}


int 
cli_set_objopt(void *obj, cli_option_t *opts, int nopt)
{
	cli_object_t *object = obj1;

	if (!obj || !opt || nopt < 1)
		return -1;

	obj1 = (cli_object_t *)obj;
	assert(obj1->magic == CLI_OBJ_MAGIC);

	obj1->opts = malloc(sizeof(cli_option_t) * nopt);
	if (!object->opts) {
		g_cli_errno = CLI_MALLOC_FAILED;
		return -1;
	}
	memset(object->opts, 0, sizeof(cli_option_t) * nopt);
	
	memcpy(object->opts, opts, sizeof(cli_option_t) * nopt);

	object->nopt = nopt;

	return 0;
}


void
cli_print_object(void *obj)
{
	if (obj) {
		_cli_print_tree(obj, 1);
	}
	else {
		_cli_print_tree(cli_rootobj, 0);
	}
}



