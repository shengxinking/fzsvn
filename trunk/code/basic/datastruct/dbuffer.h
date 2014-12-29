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

#include "gcc_common.h"

/* get data in dbuf @buf */
#define	DBUF_LEN(buf)	((buf)->len)

/* get len in dbuf @buf */
#define	DBUF_DATA(buf)	((buf)->buf)


#define	DBUF_IS_ALLOCED	0x1		/* dbuf calloced by call dbuf_alloc@ */

/**
 *	Dynamic buffer structure
 */
typedef dbuf {
	u_int8_t	*buf;
	u_int32_t	max;
	u_int32_t	len;
	u_int32_t	flags;
	u_int32_t	_pad;
} dbuf_t;


/**
 *	Alloc a new dynamic buffer and return it, it can
 *	contain @max bytes data.
 *
 * 	Return pointer if success, NULL on error.
 */
static inline dbuf_t * 
dbuf_alloc(u_int32_t max)
{
	dbuf_t *buf;

	if (unlikely(max < 1))
		return NULL;

	buf = malloc(max + sizeof(dbuf_t));
	if (unlikely(!buf))
		return NULL;

	buf->flags = DBUF_IS_ALLOCED;
	buf->len = 0;
	buf->max = max;
	buf->buf = (u_int8_t *)buf + sizeof(dbuf_t);

	return buf;
}

/**
 *	Initiate a static dynamic buffer
 *
 */
static inline int 
dbuf_init(dbuf_t *buf, u_int32_t max)
{
	if (unlikely(!buf || max < 1))
		return -1;

	buf->buf = malloc(max);
	if (!buf->buf)
		return -1;

	buf->flag = 0;
	buf->len = 0;
	buf->max = max;

	return 0;
}

static inline int 
dbuf_reset(dbuf_t *buf)
{
	if (!buf)
		return -1;

	buf->len = 0;
}

static inline int 
dbuf_free(dbuf_t *buf)
{
	if (!buf)
		return -1;

	if (buf->buf != ((u_int8_t *)buf + sizeof(dbuf_t)))
		free(buf->buf);

	if (dbuf->flags & DBUF_IS_ALLOCED)
		free(buf);

	return 0;
}

static inline int 
dbuf_resize(dbuf_t *buf, u_int32_t max)
{
	u_int8_t *nbuf;

	if (unlikely(!buf || max < 1)
		return -1;

	if (max < buf->len)
		return -1;

	if (max == buf->max)
		return 0;

	nbuf = malloc(max);
	if (unlikely(!nbuf))
		return -1;

	if (buf->buf && buf->len > 0)
		memcpy(nbuf, buf->buf, buf->len);

	if (buf->buf != ((u_int8_t *)buf + sizeof(dbuf_t)))
		free(buf->buf);

	buf->buf = nbuf;

	return 0;
}

static inline int 
dbuf_put(dbuf_t *buf, const u_int8_t *d, u_int32_t len)
{
	if (unlikely(!buf || !d || len < 1))
		return -1;

	if (!buf->buf || (buf->max - buf->len) < len)
		return -1;

	memcpy(buf->buf + buf->len, d, len);
	buf->len += len;

	return 0;
}


#endif /* end of FZ_DBUFFER_H */


