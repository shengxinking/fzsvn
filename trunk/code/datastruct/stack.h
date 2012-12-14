/*
 *	stack.h:	the stack using linked-list implement.
 *
 *	authro:		forrest.zhang
 */

#ifndef _FZ_STACK_H
#define _FZ_STACK_H

typedef struct stack_node {
	void 			*data;
	struct stack_node	*next;
} stack_node_t;

typedef struct stack {
	int			size;
	stack_node_t		*top;
	stack_node_t		*tail;
} stack_t;


extern stack_t *stack_alloc(void);

extern void stack_free(stack_t *stack);

extern int stack_push(stack_t *stack, stack_node_t *node);
extern stack_node_t *stack_pop(stack_t *stack);


#endif /* end of _FZ_STACK_H */

