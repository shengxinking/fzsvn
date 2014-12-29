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

#include "ssl_decode.h"

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


