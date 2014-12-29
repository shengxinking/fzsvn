/*
 * @file        hash.c
 * @brief       a simple hash implement
 * 
 * @author      Forrest.zhang
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "hash.h"

#define HT_SIZE_SZ      44

#define	_HH_ERR(fmt, args...)					\
	fprintf(stderr, "[%s:%d] "fmt, 				\
		__FILE__, __LINE__, ##args)

#define	_HH_ERR_RET(ret, fmt, args...)				\
({								\
	printf("[%s:%d]: "fmt, 					\
		__FILE__, __LINE__, ##args);			\
	return ret;						\
})


#define	_HH_LOCK_RET(ret, hash)					\
({								\
 	if (hash->need_lock && 					\
	    pthread_mutex_lock(&((hash_t*)(hash))->lock))	\
	{							\
		_HH_ERR("lock hash failed\n");			\
		return ret;					\
	}							\
})

#define	_HH_UNLOCK(hash)						\
({								\
 	if (hash->need_lock && 					\
	    pthread_mutex_unlock(&((hash_t *)(hash))->lock))	\
		_HH_ERR("unlock hash failed\n");		\
})


/* hash table size */
static u_int32_t ht_size[HT_SIZE_SZ] = 
{
	7, 47, 79, 97, 163, 197, 331, 397, 673, 797, 1361, 1597,
	2729, 3203, 5471, 6421, 10949, 12853, 21911, 25717, 43853,
	51437, 87719, 102877, 175447, 205759, 350899, 411527, 701819,
	823117, 1403641, 1646237, 2807303, 3292489, 5614657, 6584983,
	11229331, 13169977, 22458671, 26339969, 44917381, 52679969,
	89834777, 105359939
};


static int 
_hash_choose_size(u_int32_t size)
{
	int top;
	int mid;
	int bottom;
	u_int32_t newsize;
	
	top = HT_SIZE_SZ - 1;
	bottom = 0;
	while (bottom <= top) {

		mid = (bottom + top) / 2;
		if (size == ht_size[mid]) 
		{
			newsize = ht_size[mid];	
			break;
		}
		else if (size > ht_size[mid] && mid < HT_SIZE_SZ) 
		{
			if(size <= ht_size[mid+1]) 
			{
				newsize = ht_size[mid + 1];
				break;
			}
   			else {
				bottom = mid +1;
			}
		}
		else if (size < ht_size[mid] && mid > 0) {
			if(size >= ht_size[mid-1]) {
				if (size == ht_size[mid - 1])
					newsize = size;
				else
					newsize = ht_size[mid];
				break;
			}
			else {
				top = mid -1;
			}
		}
		else if(mid == top && mid == bottom) {
			break;
		}
	}

	if (mid == 0)
		newsize = ht_size[0];
	else if (mid == HT_SIZE_SZ - 1)
		newsize = ht_size[HT_SIZE_SZ - 1];

	return newsize;	
}

hash_t *
hash_alloc(u_int32_t size, hash_cmp_func cmp_func, int need_lock)
{
	int i;
	hash_t *hash;
	pthread_mutexattr_t attr;

	if (size < 1 || !cmp_func)
		_HH_ERR_RET(NULL, "invalid argument\n");

	hash = malloc(sizeof(hash_t));
	if (!hash) {
		_HH_ERR_RET(NULL, "malloc memory for hash error\n");
		return NULL;
	}
	memset(hash, 0, sizeof(*hash));
	
	hash->pool = objpool_alloc(sizeof(hash_item_t), 500, 0);
	if (!hash->pool) {
		free(hash);
		_HH_ERR_RET(NULL, "alloc object pool for hash_item failed\n");
	}

	hash->nentry = _hash_choose_size(size);
	hash->entries = malloc(sizeof(hash_entry_t) * hash->nentry);
	if (!hash->entries) {
		objpool_free(hash->pool);
		free(hash);
		_HH_ERR_RET(NULL, "alloc memory for hash entries failed\n");
	}
	for (i = 0; i < hash->nentry; i++)
		CBLIST_INIT(&hash->entries[i].list);

	if (need_lock) {
		hash->need_lock = 1;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
		pthread_mutex_init(&hash->lock, &attr);
		pthread_mutexattr_destroy(&attr);
	}

	hash->cmp_func = cmp_func;

	return hash;
}

int 
hash_free(hash_t *hash, hash_free_func free_func)
{
	int i;
	hash_item_t *item;
	hash_entry_t *entry;

	if (!hash)
		_HH_ERR_RET(-1, "invalid argument\n");

	_HH_LOCK_RET(-1, hash);

	if (free_func) {
		for (i = 0; i < hash->nentry; i++) {
			entry = &hash->entries[i];
			CBLIST_FOR_EACH(&entry->list, item, list) {
				free_func(item->data);
			}
		}
	}

	/* free hash_item pool */
	if (hash->pool) {
		objpool_free(hash->pool);
		hash->pool = NULL;
	}
	
	/* free hash_entry */
	if (hash->entries) {
		free(hash->entries);
		hash->entries = NULL;
	}

	_HH_UNLOCK(hash);

	free(hash);

	return 0;
}

void * 
hash_search(hash_t *hash, u_int32_t key, const void *val)
{
	int index;
	void *ret;
	hash_item_t *item;
	hash_entry_t *entry;

	if (!hash || !val)
		_HH_ERR_RET(NULL, "invalid argument\n");

	index = key % hash->nentry;
	entry = &hash->entries[index];

	ret = NULL;

	_HH_LOCK_RET(NULL, hash);
	
	CBLIST_FOR_EACH(&entry->list, item, list) {
		if (hash->cmp_func(item->data, val)) {
			hash->stat.nconflict++;
			continue;
		}
		
		ret = item->data;
		hash->stat.nfind_success++;
		break;
	}

	if (ret == NULL)
		hash->stat.nfind_failed++;

	_HH_UNLOCK(hash);

	return ret;
}

int 
hash_insert(hash_t *hash, u_int32_t key, void *val)
{
	int index;
	hash_item_t *item;
	hash_entry_t *entry;

	if (!hash || !val)
		_HH_ERR_RET(-1, "invalid argument\n");

	index = key % hash->nentry;
	entry = &hash->entries[index];

	/* add to hash entry */
	_HH_LOCK_RET(-1, hash);

	/* get new item */
	item = objpool_get(hash->pool);
	if (!item) {
		_HH_ERR("get hash_item from pool failed\n");
		_HH_UNLOCK(hash);
		return -1;
	}
	item->data = val;
	CBLIST_ADD_TAIL(&entry->list, &item->list);
	hash->size++;

	_HH_UNLOCK(hash);

	return 0;
}

int
hash_insert_nodup(hash_t *hash, u_int32_t key, void *val)
{
	int ret;
	int index;
	hash_item_t *item;
	hash_entry_t *entry;

	if (!hash || !val)
		_HH_ERR_RET(-1, "invalid argument\n");

	index = key % hash->nentry;
	entry = &hash->entries[index];
		
	ret = 0;

	_HH_LOCK_RET(-1, hash);

	/* check dupped item */
	CBLIST_FOR_EACH(&entry->list, item, list) {
		if (hash->cmp_func(item->data, val) == 0) {
			ret = -1;
			break;
		}
	}

	/* add to hash entry */
	if (ret == 0) {		
		item = objpool_get(hash->pool);
		if (item) {
			item->data = val;
			CBLIST_ADD_TAIL(&entry->list, &item->list); 
			hash->size++;
		}
		else {
			_HH_ERR("alloc hash_item from pool failed\n");
			ret = -1;
		}
	}

	_HH_UNLOCK(hash);

	return ret;
}

void *
hash_delete(hash_t *hash, u_int32_t key, const void *val)
{
	int index;
	void *ret;
	hash_item_t *item;
	hash_item_t *bak;
	hash_entry_t *entry;

	if (!hash || !val)
		_HH_ERR_RET(NULL, "invalid argument\n");

	index = key % hash->nentry;
	entry = &hash->entries[index];

	ret = NULL;

	_HH_LOCK_RET(NULL, hash);
	
	CBLIST_FOR_EACH_SAFE(&entry->list, item, bak, list) {
		if (hash->cmp_func(item->data, val)) {
			hash->stat.nconflict++;
			continue;
		}
		
		ret = item->data;
		hash->stat.nfind_success++;
		CBLIST_DEL(&item->list);
		objpool_put(item);
		hash->size--;
		break;
	}

	if (ret == NULL)
		hash->stat.nfind_failed++;

	_HH_UNLOCK(hash);

	return ret;
}

int 
hash_size(const hash_t *hash)
{
	int ret;

	if (!hash)
		_HH_ERR_RET(-1, "invalid agrument");

	_HH_LOCK_RET(-1, hash);
	ret = (int) hash->size;
	_HH_UNLOCK(hash);

	return ret;
}

int 
hash_print(const hash_t *hash, hash_print_func print_func)
{
	int i;
	int j;
	hash_item_t *item;
	hash_entry_t *entry;

	if (!hash)
		_HH_ERR_RET(-1, "invalid argument\n");

	_HH_LOCK_RET(-1, hash);

	printf("========> hash %p\n", hash);
	printf("size %d, pool %p, nentry %u, need_lock %d\n", 
	       hash->size, 
	       hash->pool, 
	       hash->nentry, 
	       hash->need_lock);

	printf("stat: nconflict %u, nfind_success %u, nfind_failed %u\n", 
	       hash->stat.nconflict, 
	       hash->stat.nfind_success, 
	       hash->stat.nfind_failed);

	for (i = 0; i < hash->nentry; i++) {
		entry = &hash->entries[i];
		
		if (CBLIST_IS_EMPTY(&entry->list))
			continue;

		printf(">>> entry(%d): %p\n", i, entry);

		j = 0;		
		CBLIST_FOR_EACH(&entry->list, item, list) {
			printf("item(%d): data %p\n", j, item->data);
			if (print_func)
				print_func(item->data);		
			j++;
		}
		
		printf("<<< entry(%d): %p\n", i, entry);
	}

	printf("<======== hash %p\n", hash);

	_HH_UNLOCK(hash);

	return 0;
}

