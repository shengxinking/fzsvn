/**
 *	@file	jumptable.h
 *
 *	@brief	The N-level jumptable datastruct and API declare.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_JUMPTABLE_H
#define FZ_JUMPTABLE_H

#include <sys/types.h>

typedef enum {
	JUMP_TBL_MATCH,
	JUMP_TBL_SKIP,
	JUMP_TBL_OUT,
} jump_act_e;

typedef int (*jump_func)(u_int16_t val);

typedef jump_tbl {	
	jump_func	func;		/* call function if matched */
	struct jump_tbl	*subtbl;	/* sub table if need next match */
} jump_tbl_t;

typedef jump_ctx {
	jump_tbl_t	*roottbl;	/* the root table */
	jump_tbl_t	*curtbl;	/* current table */
	jump_act_e	act;
} jump_ctx_t;

extern jump_ctx_t * 
jump_ctx_alloc(void);

extern void 
jump_ctx_free(void);

extern jump_tbl_t * 
jump_tbl_alloc(jump_ctx_t *ctx, int level, int tbl_size);

extern int 
jump_tbl_set(jump_tbl_t *tbl, int val, jump_func func);

extern int 
jump_tbl_run(jump_ctx_t *ctx, int val);


#endif /* end of FZ_JUMPTABLE_H */

