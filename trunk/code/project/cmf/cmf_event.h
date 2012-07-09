/**
 *	@file	cmf_event.h
 *
 *	@brief	the CMF event APIs.
 */

#ifndef	CMF_EVENT_H
#define	CMF_EVENT_H

typedef	void (*cmf_event_func)(cmf_event_t *ev);

typedef struct cmf_event {
	cmf_oid_t	oid;		/* object id */
	int		from;		/* change from */
	void 		*oldval;	/* the old value */
	void		*newval;	/* the new value */
} cmf_event_t;

extern int 
cmf_init_event(const char *progname, pid_t pid);

extern int 
cmf_add_event(int fd, cmf_oid_t oid, int events, int flags, cmf_event_func func);

extern int 
cmf_delete_event(cmf_oid_t oid);

extern int 
cmf_free_events(pid_t pid);

extern int 
cmf_event_loop(void);


#endif	/* end of CMF_EVENT_H */

