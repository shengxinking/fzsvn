/**
 *	@file	cmf_query.h
 *
 *	@brief	CMF query APIs.
 *
 */

#ifndef	CMF_QUERY_H
#define	CMF_QUERY_H


typedef struct cmf_query {
	cmf_oid_t	rootoid;
	cmf_oid_t	fatheroid;
	cmf_oid_t	oid;
	int		level;
	int		pos;
	int		from;
	void		*data;
} cmf_query_t;

extern cmf_query_t *
cmf_create_query(cmf_oid_t ngid, int from);

extern void 
cmf_free_query(cmf_query_t *q);

extern cmf_query_t *
cmf_create_child_query(cmf_query_t *q);

extern void *
cmf_query_data(cmf_query_t *q);

extern void *
cmf_query_table_find(cmf_query_t *q, cmf_oid_t id, void *key);

extern void *
cmf_query_table_first(cmf_query_t *q);

extern void *
cmf_query_table_next(cmf_query_t *q);

extern void *
cmf_query_table_end(cmf_query_t *q);

extern void *
cmf_query_table_isend(cmf_query_t *q);

extern int 
cmf_query_table_insert(cmf_query_t *q, int id, void *data);

extern int 
cmf_query_table_append(cmf_query_t *q, void *data);

extern int 
cmf_query_table_delete(cmf_query_t *q, int id);

extern int 
cmf_query_table_flush(cmf_query_t *q);

extern int 
cmf_query_commit(cmf_query_t *q);

#endif	/* end of CMF_QUERY_H */

