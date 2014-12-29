/**
 *	@file	dbuffer.h
 *
 *	@brief	Simple dynamic buffer APIs.
 *	
 *	@author	Forrest.zhang
 *
 *	@date	2014-12-26
 */

#ifndef FZ_DBUFFER_H
#define FZ_DBUFFER_H

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "gcc_common.h"

/* get data in dbuf @buf */
#define	DBUF_LEN(buf)	((buf)->len)

/* get len in dbuf @buf */
#define	DBUF_DATA(buf)	((buf)->buf)

#define	DBUF_RESET(buf)	((buf)->len = 0)

/**
 *	Dynamic buffer structure
 */
typedef struct dbuf {
	u_int32_t	max;
	u_int32_t	len;
	u_int8_t	*buf;
} dbuf_t;


/**
 *	Alloc a new dynamic buffer and return it, it can
 *	contain @max bytes data.
 *
 * 	Return pointer if success, NULL on error.
 */
static inline int  
dbuf_init(dbuf_t *buf, u_int32_t max)
{
	if (unlikely(max < 1))
		return -1;

	buf->buf = malloc(max);
	if (unlikely(!buf->buf)) 
		return -1;
	buf->len = 0;
	buf->max = max;

	return 0;
}

static inline void 
dbuf_free(dbuf_t *buf)
{
	if (unlikely(!buf))
		return;

	if (buf->buf)
		free(buf->buf);

	memset(buf, 0, sizeof(*buf));
}

static inline int 
dbuf_resize(dbuf_t *buf, u_int32_t max)
{
	u_int8_t *p;

	if (unlikely(!buf || max < 1))
		return -1;

	if (max < buf->len)
		return -1;

	if (max == buf->max)
		return 0;

	p = malloc(max);
	if (unlikely(!p))
		return -1;

	if (buf->buf && buf->len > 0)
		memcpy(p, buf->buf, buf->len);

	free(buf->buf);
	buf->buf = p;

	return 0;
}

static inline int 
dbuf_put(dbuf_t *buf, const u_int8_t *d, u_int32_t len)
{
	if (unlikely(!buf || !d || len < 1))
		return -1;

	/* no space to put data */
	if (!buf->buf || (buf->max - buf->len) < len)
		return -1;

	memcpy(buf->buf + buf->len, d, len);
	buf->len += len;

	return 0;
}


#endif /* end of FZ_DBUFFER_H */


