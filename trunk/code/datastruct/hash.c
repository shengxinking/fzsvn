/*
 * @file        hash.c
 * @brief       a simple hash implement
 * 
 * @author      Forrest.zhang
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "hash.h"
#include "debug.h"

#define HASH_NOKEY      0xFFFFFFFF
#define HT_SIZE_SZ      44
static u_int32_t ht_size[HT_SIZE_SZ] = {
	7, 47, 79, 97, 163, 197, 331, 397, 673, 797, 1361, 1597,
	2729, 3203, 5471, 6421, 10949, 12853, 21911, 25717, 43853,
	51437, 87719, 102877, 175447, 205759, 350899, 411527, 701819,
	823117, 1403641, 1646237, 2807303, 3292489, 5614657, 6584983,
	11229331, 13169977, 22458671, 26339969, 44917381, 52679969,
	89834777, 105359939
};


static int _hash_choose_size(size_t size)
{
	int bottom = 0, top = HT_SIZE_SZ -1, mid;
	int found = 0;
	unsigned int ret = 0x7FFFFFFF;
	
	while (bottom <= top && !found) {

		mid = (bottom + top) / 2;
		if(size == ht_size[mid]) {
			found = 1;
			ret = ht_size[mid];	
		}
		else if(size >  ht_size[mid] && mid < HT_SIZE_SZ) {
			if(size <= ht_size[mid+1]) {
				found = 1;
				ret = ht_size[mid+1];
			}
   			else {
				bottom = mid +1;	
			}
		}
		else if(size <  ht_size[mid] && mid > 0) {
			if(size >= ht_size[mid-1]) {
				found = 1;
				if (size == ht_size[mid - 1])
					ret = size;
				else
					ret = ht_size[mid];
			}
			else {
				top = mid -1;	
			}	
		}
		else if(mid == top && mid == bottom) {
			break;
		}
	}


	if(found) {
		return ret;	
	}
	else {
		switch(mid) {
		
		case 0: 
			return ht_size[0];
		
		case HT_SIZE_SZ - 1 : 
			return ht_size[HT_SIZE_SZ - 1];
		
		default:	
			return 0x7FFFFFFF;
		} 
	}
}


static void _hash_lock(hash_t *hash)
{

#ifdef _REENTRANT

	if (!hash->lock_times || hash->lock_tid != pthread_self())
		pthread_mutex_lock(&hash->lock);

#endif /* for thread safe */

}

static void _hash_unlock(hash_t *hash)
{

#ifdef _REENTRANT

	if (!hash->lock_times || hash->lock_tid != pthread_self())
		pthread_mutex_unlock(&hash->lock);

#endif /* for thread safe */

}

static int _hash_find(hash_t *hash, u_int32_t hkey, u_int32_t key, const void *var)
{
	int ret = HASH_EOF;
	hash_item_t *pool = hash->pool;
	int index;
	int n, i;

	index = hash->hash[hkey].index;
	if (!var) {
		while (pool[index].next != HASH_EOF && pool[index].key != key)
			index = pool[index].next;
		
		if (pool[index].key == key)
			ret = index;
	}
	else {
		if (!hash->cmp_func)
			return ret;

		n = hash->hash[index].size;
		for (i = 0; i < n; i++) {
			if (pool[index].key == key &&
			    hash->cmp_func(pool[index].data, var) == 0) {
				ret = index;
				break;
			}

		}
		index = pool[index].next;
	}

	return ret;
}

int hash_init(hash_t *hash, size_t size, hash_cmp_func cmp_func)
{
	int i;

	if (!hash || size < 1)
		return -1;

	/* alloc pool */
	hash->pool = malloc(sizeof(hash_item_t) * size);
	if (!hash->pool) {
		ERR("malloc memory error\n");
		return -1;
	}

	/* alloc hash entry */
	hash->hash = malloc(sizeof(hash_entry_t) * size);
	if (!hash->pool) {
		ERR("malloc memory error\n");
		free(hash->pool);
		return -1;
	}

	/* init hash */
	for (i = 1; i < size - 1; i++) {
		hash->hash[i].size = 0;
		hash->hash[i].index = HASH_EOF;

		hash->pool[i].prev = i - 1;
		hash->pool[i].next = i + 1;
		hash->pool[i].key = HASH_NOKEY;
		hash->pool[i].data = NULL;
	}

	hash->hash[0].size = 0;
	hash->hash[0].index =HASH_EOF;

	hash->hash[size - 1].size = 0;
	hash->hash[size - 1].index = HASH_EOF;

	hash->pool[0].prev = HASH_EOF;
	hash->pool[0].next = 1;
	hash->pool[0].key = HASH_NOKEY;
	hash->pool[0].data = NULL;

	hash->pool[size - 1].prev = size - 2;
	hash->pool[size - 1].next = HASH_EOF;
	hash->pool[size - 1].key = HASH_NOKEY;
	hash->pool[size - 1].data = NULL;

	hash->size = size;
	hash->used = 0;
	hash->free_index = 0;
	hash->cmp_func = cmp_func;

#ifdef _REENTRANT

	pthread_mutex_init(&hash->lock, NULL);
	hash->lock_times = 0;
	hash->lock_tid = 0;

#endif /* for thread safe */

	return 0;
}


hash_t *hash_alloc(size_t size, hash_cmp_func cmp_func)
{
	hash_t *hash;

	if (size < 1)
		return NULL;

	size = _hash_choose_size(size);
	
	hash = malloc(sizeof(hash_t));
	if (!hash) {
		ERR("malloc memory error\n");
		return NULL;
	}

	if (hash_init(hash, size, cmp_func)) {
		free(hash);
		return NULL;
	}

	return hash;
}

void hash_free(hash_t *hash)
{
	if (!hash)
		return;

	if (hash->pool)
		free(hash->pool);
	
	if (hash->hash)
		free(hash->hash);

	free(hash);
}


void *hash_get(hash_t *hash, u_int32_t key, const void *var)
{
	int index;
	u_int32_t hv;
	void *ret;
	int pos;

	if (!hash)
		return NULL;

	_hash_lock(hash);

	hv = key % hash->size;
	index = hash->hash[key].index;
	if (index == HASH_NOKEY || 
	    (pos = _hash_find(hash, hv, key, var)) < 0)
		ret = NULL;
		
	else
		ret = (void *)hash->pool[pos].data;
	
	_hash_unlock(hash);

	return ret;
}

static int _hash_put(hash_t *h, u_int32_t hkey, u_int32_t key, const void *d)
{
	hash_item_t *pool = h->pool;
	int free_index;
	int hash_index;
	int index;

	hash_index = h->hash[hkey].index;
	free_index = h->free_index;
	
	index = free_index;
	free_index = pool[free_index].next;
	if (free_index != HASH_EOF)
		pool[free_index].prev = HASH_EOF;

	if (hash_index != HASH_EOF)
		pool[hash_index].prev = index;

	pool[index].next = hash_index;
	pool[index].prev = HASH_EOF;
	pool[index].data = d;
	pool[index].key = key;

	h->hash[hkey].index = index;
	h->hash[hkey].size++;
	h->free_index = free_index;
	h->used++;

	return 0;
}

int hash_put(hash_t *hash, u_int32_t key, const void *data)
{
	int ret = 0;
	u_int32_t hkey;

	if (!hash || !data)
		return -1;

	hkey = key % hash->size;
	_hash_lock(hash);

	if (hash->free_index == HASH_EOF)
		ret = -1;
	else 
		ret = _hash_put(hash, hkey, key, data);

	_hash_unlock(hash);

	return ret;
}



void *hash_remove(hash_t *hash, u_int32_t key, const void *var)
{
	u_int32_t hv;
	int pos;
	int index;
	int prev, next;
	void *ret;
	hash_item_t *pool;
	
	if (!hash)
		return NULL;

	hv = key % hash->size;
	_hash_lock(hash);

	index = hash->hash[hv].index;
	if (index == HASH_EOF || (pos = _hash_find(hash, hv, key, var)) < 0)
		ret = NULL;
	else {
		pool = hash->pool;
		prev = pool[pos].prev;
		next = pool[pos].next;
		if (prev != HASH_EOF)
			pool[prev].next = next;
		else
			hash->hash[hv].index = next;
		
		if (next != HASH_EOF)
			pool[next].prev = prev;

		ret = (void *)pool[pos].data;
		
		if (hash->free_index != HASH_EOF)
			pool[hash->free_index].prev = pos;

		pool[pos].next = hash->free_index;
		pool[pos].prev = HASH_EOF;
		pool[pos].key = HASH_NOKEY;
		pool[pos].data = NULL;

		hash->free_index = pos;
		hash->used--;
		hash->hash[hv].size--;
	}

	_hash_unlock(hash);

	return ret;
}


void *hash_change(hash_t *hash, u_int32_t key, const void *var, const void *new)
{
	int index;
	u_int32_t hv;
	void *ret;
	int pos;

	if (!hash)
		return NULL;

	_hash_lock(hash);

	hv = key % hash->size;
	index = hash->hash[key].index;
	if (index == HASH_NOKEY || 
	    (pos = _hash_find(hash, hv, key, var)) < 0)
		ret = NULL;
		
	else {
		ret = (void *)hash->pool[pos].data;
		hash->pool[pos].data = new;
	}

	_hash_unlock(hash);

	return ret;
}

void hash_lock(hash_t *hash)
{

#ifdef _REENTRANT
	
	if (!hash->lock_times || hash->lock_tid != pthread_self()) {
		pthread_mutex_lock(&hash->lock);
		hash->lock_times = 1;
		hash->lock_tid = pthread_self();
	}
	else {
		hash->lock_times++;
	}
	
#endif /* for thread safe */

}

void hash_unlock(hash_t *hash)
{

#ifdef _REENTRANT
	
	if (--hash->lock_times == 0)
		pthread_mutex_unlock(&hash->lock);

#endif /* for thread safe */

}

int hash_count(const hash_t *hash)
{
	int ret;

	if (!hash)
		return -1;

	_hash_lock((void *)hash);

	ret = hash->size;

	_hash_unlock((void *)hash);

	return ret;
}


void hash_print(const hash_t *hash, hash_print_func print_func)
{
	int index;
	hash_item_t *pool;
	int i;

	if (!hash)
		return;

	_hash_lock((void *)hash);

	pool = hash->pool;
	index = hash->free_index;
	printf("hash(%p): size %u, used %u, free_index %d, pool %p, hash %p\n",
	       hash, hash->size, hash->used, index, pool, hash->hash);

	
	printf("free list:\n");
	while(index != HASH_EOF) {

		printf("\titem(%p): key 0x%X, prev %d, next %d, data %p\n",
		       &pool[index], pool[index].key, pool[index].prev, 
		       pool[index].next, pool[index].data);

		index = pool[index].next;
	}

	printf("hash:\n");
	for (i = 0; i < hash->size; i++) {
		
		printf(">>> entry(%p): size %u, index %d\n", &hash->hash[i],
		       hash->hash[i].size, hash->hash[i].index);
		
		index = hash->hash[i].index;
		if (index == HASH_EOF) {
			printf("\tno items\n");
			printf("<<< end of entry\n");
			continue;
		}
		
		while (index != HASH_EOF) {
			printf("key 0x%X, prev %d, next %d\n",
			       pool[index].key, pool[index].prev, 
			       pool[index].next);
			
			if (print_func) {
				printf("data: \n");
				print_func(pool[index].data);
			}
			else 
				printf("data %p\n", pool[index].data);

			index = pool[index].next;
		}

		printf("<<< end of entry\n");
	}

	_hash_unlock((void *)hash);
}

