/** 
 *	@file	cmf_object.h
 *
 *	@brief	cmf node/object APIs.
 *
 *	@author	Forrest.zhang
 */

#ifndef FZ_CMF_OBJECT_H
#define FZ_CMF_OBJECT_H

/* cmf node magic number */
#define CMF_NODE_MAGIC	0x13579BD0

/* cmf node type */
enum 
{
	CMF_PATH,
	CMF_SINGLE,
	CMF_TABLE,
	CMF_ATTR,
	CMF_EXEC,
	CMF_DIAG,
};

/* cmf node functions */
typedef struct cmf_node_ops 
{
	int		nid;		/* the node ID */
	cmf_config_func config_func;
	cmf_end_func	end_func;
	cmf_exit_func	exit_func;
	cmf_abort_func	about_func;
	cmf_set_func	set_func;
	cmf_unset_func	unset_func;
	cmf_get_func	get_func;
	cmf_parse_func	parse_func;
	cmf_edit_func	edit_func;
	cmf_del_func	del_func;
	cmf_get_func	get_func;
	cmf_show_func	show_func;
	cmf_print_func	print_func;
} cmf_node_ops_t;

/* cmf node */
typedef struct cmf_node 
{
	char		name[CMF_NAMELEN];/* object name */
	char		desc[CMF_DESCLEN];/* object comment */
	u_int32_t	magic;		/* object magic number */
	u_int32_t	nid;		/* the unique id for node */
	int		type;		/* node type */
	int		dtype;		/* data type */
	u_int16_t	offset;		/* node offset, for attr */
	u_int16_t	size;		/* object storage size */
	u_int16_t	group;		/* support max 255 group */
	u_int8_t	perm;		/* object permision */
	u_int8_t	flags;		/* flags: HA */
	u_int16_t	childs[CMF_CHILDMAX];/* the childs of objects */
	u_int16_t	nchild;		/* childs number */
	u_int16_t	options[CLI_OPTMAX];/* option values */
	u_int16_t	noption;	/* Option value for Attribute */
	u_int32_t	parent;		/* the parent of this object */
	u_int32_t	sibling;	/* next sibling node */
} cmf_node_t;

/* cmf value */
typedef struct cmf_value 
{
	int		nid;		/* node id */
	int		vid;		/* value id */
	int		len;		/* the length */
	char		buf[0];		/* the value data */
} cmf_value_t;

/* cmf multi-option */
typedef struct cmf_mulopt 
{
	char		name[CMF_NAMELEN];
	u_int32_t	value;
	u_int32_t	selected;
} cmf_mulopt_t;

/* cmf single option */
typedef struct cmf_opt 
{
	char		name[CMF_NAMELEN];
	int		value;	
} cmf_opt_t;


extern int 
cmf_add_node(cmf_node_t *node);

extern int 
cmf_add_attr(cmf_node_t *node, cmf_node_t *attr);

extern int 
cmf_add_opt(cmf_node_t *node, cmf_opt_t *opt);

extern int 
cmf_add_mulopt(cmf_node_t *node, cmf_mulopt_t *opt);


#endif /* end of FZ_CLI_OBJECT_H */

