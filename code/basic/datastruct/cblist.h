/**
 *	@file	cblist.h
 *
 *	@brief	The circle bi-direction list implement, copy from haproxy.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_CBLIST_H
#define FZ_CBLIST_H

/**
 *	cblist structure
 */
typedef struct cblist {
	struct cblist	*p;	/* previous element */
	struct cblist	*n;	/* next element */
} cblist_t;

/**
 *	Initiate cblist_t @l.
 */
#define	CBLIST_INIT(l)		((l)->p = (l)->n = (l))

/**
 *	Get object contained the cblist member @l, the object type is @t, 
 *	member name is @m.
 */
#define	CBLIST_ELEM(l, t, m)	((t)(((void *)(l)) - ((void *)&((t)NULL)->m)))

/**
 *	Check cblist @l linked into a list or not. if TRUE, not 
 *	linked or linked
 */
#define	CBLIST_IS_EMPTY(l)	((l)->n == (l))

/**
 *	Get first object in cblist @lh, the object type is @t, the 
 *	cblist member name is @m.
 */
#define	CBLIST_GET_HEAD(lh, t, m)			\
	((lh)->n == (lh) ? NULL : CBLIST_ELEM((lh)->n, t, m))

/**
 *	Get last object in cblist @lh, the object type is @t, the
 *	cblist member name in structure @t is @m.
 */
#define	CBLIST_GET_TAIL(lh, t, m)			\
	((lh)->p == (lh) ? NULL : CBLIST_ELEM((lh)->n, t, m))

/**
 *	Add cblist @l to head of cblist @lh
 */
#define	CBLIST_ADD_HEAD(lh, l)				\
	({						\
	 	(l)->n = (lh->n);			\
	 	(l)->n->p = (lh)->n = (l);		\
	 	(l)->p = (lh);				\
	})

/**
 *	Add cblist @l to tail of cblist @lh
 */
#define	CBLIST_ADD_TAIL(lh, l)				\
	({						\
	 	(l)->p = (lh)->p;			\
	 	(l)->p->n = (lh)->p = (l);		\
	 	(l)->n = (lh);				\
	})

/**
 *	Del cblist @l from list.
 */
#define	CBLIST_DEL(l)					\
	({ 						\
	 	(l)->n->p = (l)->p;			\
	 	(l)->p->n = (l)->n;			\
	 	CBLIST_INIT(l);				\
	 })

/**
 *	List every object @e in cblist @lh, the cblist member in 
 *	structure typeof(e) is @m.
 */
#define	CBLIST_FOR_EACH(lh, e, m)			\
	for (e = CBLIST_ELEM((lh)->n, typeof(e), m);	\
	     &e->m != (lh);				\
	     e = CBLIST_ELEM(e->m.n, typeof(e), m)) 

/**
 *	List every object @e in cblist @lh, the cblist member in 
 *	structure typeof(e) is @m. object @b backup next object 
 *	position for delete object @e.
 *
 *	This foreach macro can delete element in cblist. 
 */
#define	CBLIST_FOR_EACH_SAFE(lh, e, b, m)		\
	for (e = CBLIST_ELEM((lh)->n, typeof(e), m),	\
	     b = CBLIST_ELEM(e->m.n, typeof(e), m);	\
	     &e->m != (lh);				\
	     e = b, b = CBLIST_ELEM(b->m.n, typeof(b), m))

/**
 *	Move all object in cblist @lh2 to tail of cblist @lh1. 
 */
#define	CBLIST_JOIN(lh1, lh2)				\
({							\
	if (!CBLIST_IS_EMPTY((lh2)))			\
	{						\
		(lh1)->p->n = (lh2)->n;			\
		(lh2)->n->p = (lh1)->p;			\
		(lh1)->p = (lh2)->p;			\
		(lh2)->p->n = (lh1);			\
		CBLIST_INIT((lh2));			\
	}						\
})


#endif /* end of FZ_CBLIST_H  */


