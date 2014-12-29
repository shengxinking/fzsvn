/**
 *	@file	cmf_api.h
 *
 *	@brief	The cmf apis and data structure.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2013-06-15
 */

#ifndef FZ_CMF_API_H
#define FZ_CMF_API_H

/* cmf object length */
#define	CMF_NAMELEN		32
#define	CMF_DESCLEN		64
#define	CMF_CHILDMAX		16

/* cmf data type */
enum {
	CMF_NONE,
	CMF_INT,
	CMF_SHORT,
	CMF_UINT,
	CMF_LONG,
	CMF_ULONG,
	CMF_ID,
	CMF_STR,
	CMF_TXT,
	CMF_DTMAX,
};

/* cmf object type */
enum {
	CMF_COMPLEX,
	CMF_TABLE,
};

/* cmf action */
#define	CMF_ADD			0x0001
#define CMF_DEL			0x0002
#define	CMF_EDIT		0x0004
#define	CMF_ADD_CHILD		0x0010
#define	CMF_DEL_CHILD		0x0020
#define CMF_EDIT_CHILD		0x0040
#define	CMF_ALL_CHILD_EVENTS	(CMF_ADD_CHILD|CMF_DEL_CHILD|CMD_EDIT_CHILD)
#define	CMF_ALL_EVENTS		(CMF_ADD|CMF_DEL|CMD_EDIT|CMF_CHILD_ALL)

/* cmf perm */
#define CMF_READ		0x0001
#define	CMF_WRITE		0x0002
#define	CMF_RDWR		(CMF_READ|CMF_WRITE)
#define	CMF_EXEC		0x0010
#define	CMF_DIAG		0x0020

/* cmf group */
#define	CMF_GRP_SYS		0x0001
#define	CMF_GRP_NET		0x0002
#define CMF_GRP_TRA		0x0010
#define	CMF_GRP_PROT		0x0020
#define CMF_GRP_STAT		0x0040
#define CMF_GRP_LOG		0x0100


/* cmf query */
typedef struct cmf_query 
{
	int			nid;	/* node id */
	int			vid;	/* value id */
	int			child_oid;/* child's object id */
	void			*val;	/* value */
} cmf_query_t;

/* cmf event */
typedef struct cmf_event 
{
	int			nid;
	int			vid;
	void			*oldval;
	void			*newval;
	int			action;
} cmf_event_t;


/**********************************************************
 *	CMF query APIs
 *********************************************************/

/**
 *	Create a CMF query according object ID @oid.
 *
 *	Return the query if success, NULL on failed.
 */
extern cmf_query_t *  
cmf_query_create(int oid);

/**
 *	Free the CMF query @query.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_query_free(cmf_query_t *query);

/**
 *	Return the query data, the query's object must be 
 *	complex object.
 *
 *	Return the query data if success, -1 on error.
 */
extern void * 
cmf_query_data(cmf_query_t *query);

/**
 *	Return the query's table max size.
 *
 *	Return > 0 if success, -1 on error.
 */
extern int 
cmf_query_table_size(cmf_query_t *query);

/**
 *	Return the query's table object number.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
cmf_query_table_num(cmf_query_t *query);

/**
 *	Find the value according the the object's field name @n 
 *	and value @v. 
 *
 *	Return data if success, NULL on not found or failed.
 */
extern void * 
cmf_query_table_find(cmf_query_t *query, const char *n, const char *v);

/**
 *	Add a new object @data into position @pos.
 *
 *	Return 0 if success, -1 on error.
 */
extern int  
cmf_query_table_add(cmf_query_t *query, int pos, void *data);

/**
 *	Add a new object @data into tail of table.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_query_table_append(cmf_query_t *query, void *data);

/**
 *	Delete a object at position @pos in table.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_query_table_del(cmf_query_t *query, int pos);

/**
 *	Clear all objects in table.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_query_table_clear(cmf_query_t *query);


/**********************************************************
 *	CMF event APIs
 *********************************************************/

/**
 *	Register cmf event. the programe name is @prog.
 *
 *	Return the file descript if success, -1 on error.
 */
extern int 
cmf_event_register(const char *prog);

/**
 *	Unregister cmf event. the programe name is @prog. 
 *	And close the file descript.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_event_unregister(int fd, const char *prog);

/**
 *	Add a cmf event on node @oid, the notify action is @action.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_event_add(int efd, int nid, int action);

/**
 *	Delete cmf event on node @oid.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_event_del(int efd, int nid);

/**
 *	Handle cmdb events.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
cmf_event_handle(int efd);


#endif /* end of FZ_CMF_API_H  */

