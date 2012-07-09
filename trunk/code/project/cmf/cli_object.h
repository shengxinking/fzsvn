/** 
 *	@file	cli_object.h
 *
 *	@brief	declare CLI object and it's functions.
 *
 *	@author	Forrest.zhang
 */

#ifndef FZ_CLI_OBJECT_H
#define FZ_CLI_OBJECT_H

#include "cli_command.h"
#include "cli_datatype.h"

#define CLI_NAME_MAX	32	/* the all object name max length in CLI */
#define CLI_COMMENT_MAX	64	/* the comment length in CLI object */
#define	CLI_OBJECT_MAX	2048	/* the max object number in tree */	
#define CLI_CHILD_MAX	128	/* the max child number in single object */
#define CLI_OPTION_MAX	64	/* the max option in object */


#define CLI_OFFSET(type, member) \
	( (unsigned long)(&((type *)0)->member))

#define	CLI_OBJPTR(ptr, id)			\
	(void *)((unsigned long)ptr + id * sizeof(cli_object_t))

#define CLI_OPTPTR(ptr, id)			\
	(void *)((unsigned long)ptr + id * sizeof(cli_option_t))


/**
 *	CLI object functions table
 */
typedef struct cli_objfunc {
	cli_config_func config_func;
	cli_end_func	end_func;
	cli_exit_func	exit_func;
	cli_abort_func	about_func;
	cli_set_func	set_func;
	cli_unset_func	unset_func;
	cli_get_func	get_func;
	cli_parse_func	parse_func;
	cli_edit_func	edit_func;
	cli_del_func	del_func;
	cli_show_func	show_func;
	cli_print_func	print_func;
} cli_objfunc_t;


/**
 *	CLI object type
 */
#define CLI_OBJ_PATH	0		/* Path object */
#define CLI_OBJ_SINGLE	1		/* Unique struct object */
#define CLI_OBJ_TABLE	2		/* Table struct object */
#define CLI_OBJ_ATTR	3		/* Attribute object */
#define CLI_OBJ_EXEC	4		/* Execute object */
#define CLI_OBJ_DIAG	5		/* Diagnose object */


/*	CLI object magic number */
#define CLI_OBJ_MAGIC	0x0CC0FEEF	/* object magic number */


/**
 *	CLI object define.
 */
typedef struct cli_object {
	char		name[CLI_NAME_MAX];	/* object name */
	char		comment[CLI_COMMENT_MAX];/* object comment */
	u_int32_t	magic;		/* object magic number */
	u_int16_t	id;		/* the unique id for object */
	u_int16_t	dtype;		/* data type of object */
	u_int16_t	offset;		/* object offset */
	u_int16_t	size;		/* object storage size */
	u_int8_t	type;		/* object type */
	u_int8_t	group;		/* support max 255 group */
	u_int8_t	perm;		/* object permision */
	u_int8_t	flags;		/* flags: HA */
	u_int16_t	childs[CLI_CHILD_MAX];	/* the childs of objects */
	u_int16_t	nchild;		/* childs number */
	u_int16_t	options[CLI_OPT_MAX];	/* operation function table */
	u_int16_t	noption;	/* Option value for Attribute */
	u_int16_t	parent;		/* the parent of this object */
	u_int16_t	sibling;	/* next sibling object */
} cli_object_t;


/**
 *	CLI multi-select option
 */
typedef struct cli_mulopt {
	char		name[CLI_NAME_MAX];
	u_int16_t	value;
	u_int16_t	selected;
} cli_mulopt_t;


/**
 *	Init cli object tree.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_init_objtree(void);


/**
 *	Free object tree.
 *
 *	No return.
 */
extern void 
cli_free_objtree(void);


/**
 *	Register a new object to CLI tree. It's path is @path.
 *
 *	Return object if success, NULL on error.
 */
extern void * 
cli_add_object(const char *path, const char *name, u_int8_t type, 
	       u_int16_t offset, u_int16_t size);


/**
 *	Add a attribute object to object @obj.
 *
 *	Return attribute object.
 */
extern void * 
cli_add_attribute(void *obj, const char *name, u_int16_t off, u_int16_t size);


/**
 *	Set the comment for a object.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_set_objcomment(void *obj, const char *comment);


/**
 *	Set object @obj's flags @flags.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_set_objflags(void *obj, u_int8_t flags);


/**
 *	Set object @obj's functions.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cli_set_objfunc(void *obj, cli_objfunc_t funcs); 


/**
 *	Set object @obj's permission, it's group is @group, permission
 *	is @perm.
 *
 *	Return 0 if success.
 */
extern int 
cli_set_objperm(void *obj, u_int8_t group, u_int8_t perm);


/**
 *	Print all object in tree @g_cli_root.
 *
 *	No return.
 */
extern void
cli_print_object(void *obj);


#endif /* end of FZ_CLI_OBJECT_H */

