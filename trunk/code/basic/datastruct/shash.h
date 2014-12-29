/**
 *	@file	shash.h
 *
 *	@brief	Simple hash APIs and structure.
 *	
 *	@author	Forrest.zhang
 *
 *	@date
 */

#ifndef FZ_SHASH_H
#define FZ_SHASH_H

#include "gcc_common.h"
#include "dbg_common.h"

#include "cblist.h"
#include "objpool.h"

typedef void (shash_cmp_func *)(const void *d1, const void *d2);
typedef void (shash_free_func *)(void *data);
typedef void (shash_print_func *)(const void *data);

typedef struct shash_item {
	cblist_t	list;
	void		*item;
} shash_item_t;

typedef struct shash_bucket {
	cblist_t	list;
	u_int32_t	size;
} shash_bucket_t;

typedef struct shash {
	shash_bucket_t	*buckets;		/* buckets */
	u_int32_t	nbucket;		/* number of bucket */
	objpool_t	*itempool;		/* item pool */
	u_int32_t	size;			/* items number in hash */
	shash_cmp_func	cmp_func;		/* compare function */
	shash_free_func	free_func;		/* free function */
} shash_t;


static inline shash_t *
shash_alloc(u_int32_t bucket, shash_cmp_func cfunc, shash_free_func ffunc)
{
	int i;
	shash_t *h;
	shash_bucket_t *b;

	if (unlikely(max < 1))
		ERR_RET(NULL, "invalid argument\n");

	h = malloc(sizeof(shash_t));
	if (unlikely(!h))
		ERR_RET(NULL, "malloc shash failed: %s\n", ERRSTR);

	h->buckets = malloc(sizeof(shash_bucket_t) * max);
	if (unlikely(!h->buckets)) {
		free(h);
		ERR_RET(NULL, "malloc buckets failed: %s\n", ERRSTR);
	}

	for (i = 0; i < max; i++) {
		b = &h->buckest[i];
		CBLIST_INIT(b);
		b->size = 0;
	}

	h->itempool = objpool_alloc(1024, 0);
	if (!h->itempool) {
		free(h->buckets);
		free(h);
		ERR_RET(NULL, "objpool_alloc failed\n");
	}
	h->nbucket = max;
	h->size = 0;
	h->cmp_func = cfunc;
	h->free_func = ffunc;

	return h;
}

static inline void  
shash_free(shash_t *h)
{
	int i;
	shash_item_t *item, *bak;
	shash_bucket_t *b;

	if (unlikely(!h))
		ERR_RET(-1, "invalid argument\n");

	/* free buckets items */
	if (h->buckets) {
		for (i = 0; i < h->max; i++) {
			b = &h->buckest[i];
			CBLIST_FOR_EACH_SAFE(&b->list, item, bak, list) {
				CBLIST_DEL(item);
				if (h->free_func)
					h->free_func(item->data);

				objpool_put(item);
				b->size--;
			}
			if (unlikely(h->size))
				ERR("invalid hash size after free\n");
		}
		free(h->buckets);
	}

	if (h->itempool) 
		objpool_free(h->itempool);

	free(h);
}

static inline int 
shash_add(shash_t *h, void *d, u_int32_t hval, int check)
{
	shash_item_t *item;
	shash_bucket_t *b;

	if (unlikely(!h || !d))
		ERR_RET(-1, "invalid argument\n");

	hval %= h->nbucket;
	b = h->buckets[hval];
	
	if (check) {
		CBLIST_FOR_EACH(&b->list, item, list) {
			if (item->data == d || 
			    h->cmp_func(item->data, d) == 0) {
				ERR_RET(-1, "duplicate data: %p\n", 
					item->data);
			}
		}
	}

	item = objpool_get(h->itempool);
	if (!item)
		ERR_RET(-1, "objpool_get failed\n");

	item->data = d;
	CBLIST_ADD_TAIL(&b->list, &item->list);
	b->size++;
	h->size++;
	
	return 0;
}

static inline void *  
shash_del(shash_t *h, void *d, u_int32_t hval)
{
	void *data;
	shash_item_t *item;
	shash_bucket_t *b;

	if (unlikely(!h || !d))
		ERR_RET(-1, "invalid argument\n");

	hval %= h->nbucket;
	b = h->buckets[hval];
	
	CBLIST_FOR_EACH_SAFE(&b->list, item, list) {
		if (h->cmp_func(item->data, d) == 0) {
			CBLIST_DEL(&item->list);
			data = item->data;
			objpool_put(item);
			return data;
		}
	}

	return NULL;
}

static inline void *
shash_find(shash_t *h, void *d, u_int32_t hval)
{
	void *data;
	shash_item_t *item;
	shash_bucket_t *b;

	if (unlikely(!h || !d))
		ERR_RET(-1, "invalid argument\n");

	hval %= h->nbucket;
	b = h->buckets[hval];
	
	CBLIST_FOR_EACH_SAFE(&b->list, item, list) {
		if (h->cmp_func(item->data, d) == 0) {
			CBLIST_DEL(&item->list);
			data = item->data;
			return data;
		}
	}

	return NULL;	
}

static inline void 
shash_print(const shash_t *h, shash_print_func pfunc)
{
	int i;
	shash_item_t *item;
	shash_bucket_t *b;

	if (unlikely(!h)) {
		ERR("invalid argument\n");
		return;
	}

	printf("shash(%p): buckets(%p), nbucket %u, itempool(%p), size %u\n", 
	       h, h->buckets, h->nbucket, h->itempool, h->size);

	for (i = 0; i < h->nbucket; i++) {
		b = &h->buckets[i];
		printf("\tbucket(%d): size %u\n", b->size);
		if (pfunc) {
			printf("\t\t");
			CBLIST_FOR_EACH(&b->list, item, list) {
				pfunc(item->data);
			}
		}
	}
}


#endif /* end of FZ_SHASH_H */


