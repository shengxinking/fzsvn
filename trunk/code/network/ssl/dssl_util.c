/**
 *	@file	packet_ssl.c
 *
 *	@brief	SSL packet decode functions.
 *	
 *	@author	Forrest.zhang	
 *
 *	@date	2014-12-25
 */

#include "gcc_common.h"
#include "dbg_common.h"

#include "dssl_util.h"


dssl_ctx_t *
dssl_ctx_new(void)
{
	dssl_ctx_t *ctx;

	ctx = malloc(sizeof(dssl_ctx_t));
	if (unlikely(!ctx))
		ERR_RET(-1, "malloc failed: %s\n", ERRSTR);
	memset(ctx, 0, sizeof(*ctx));

	
}


int 
dssl_decode(dssl_t *s, const u_int8_t *buf, size_t len, int dir)
{
	dbuf_t *buf;
	
	if (unlikely(!s || !buf || len < 1))
		ERR_RET(-1, "invalid argument\n");

	if (dir)
		buf = s->
	
	return 0;
}


