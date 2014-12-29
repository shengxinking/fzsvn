/*
 *	stack.h:	the stack using linked-list implement.
 *
 *	authro:		forrest.zhang
 */

#ifndef _FZ_STACK_H
#define _FZ_STACK_H

typedef struct stack {
	void		*cur;		/* current stack top */
	void		*start;		/* alloced memory end */
	void		*end;		/* alloced memory start */
	int		size;		/* size of object in stack */
	u_int16_t	need_lock:1;	/* need lock or not */
	pthread_mutex_t	lock;		/* lock for thread-safe */
} stack_t;

extern stack_t * 
stack_alloc(void);

extern void 
stack_free(stack_t *sk);

extern int 
stack_push(stack_t *sk, void *data);

extern void * 
stack_pop(stack_t *sk);

#endif /* end of _FZ_STACK_H */

