/*
 * @file        recv_hash.h
 * @brief       recv thread need a hash to map fd to stream and work thread
 *
 * @author      Forrest.zhang
 */

#ifndef FZ_HASH_H
#define FZ_HASH_H

#include <sys/types.h>

#ifdef _REENTRANT
#include <pthread.h>
#endif /* for thread safe */


typedef int (*hash_cmp_func)(const void *data, const void *arg);
typedef void (*hash_print_func)(const void *data);

#define HASH_EOF        -1

typedef struct hash_item {
	u_int32_t               key;
	int                     prev;
	int                     next;
	const void              *data;
} hash_item_t;

typedef struct hash_entry {
	u_int32_t               size;
	int                     index;
} hash_entry_t;

typedef struct hash {
	u_int32_t       size;
	u_int32_t       used;

	int             free_index;

	hash_item_t     *pool;
	hash_entry_t    *hash;

	hash_cmp_func   cmp_func;

#ifdef _REENTRANT

	pthread_mutex_t lock;
	u_int32_t       lock_times;
	pthread_t       lock_tid;

#endif /* for thread_safe */

} hash_t;

extern hash_t *hash_alloc(size_t size, hash_cmp_func cmp_func);

extern int hash_init(hash_t *hash, size_t size, hash_cmp_func cmp_func);

extern void hash_free(hash_t *hash);

extern void *hash_get(hash_t *hash, u_int32_t key, const void *var);

extern int hash_put(hash_t *hash, u_int32_t key, const void *data);

extern void *hash_remove(hash_t *hash, u_int32_t key, const void *var);

extern void hash_lock(hash_t *hash);

extern void hash_unlock(hash_t *hash);

extern int hash_size(const hash_t *hash);

extern void hash_print(const hash_t *hash, hash_print_func print_func);

#endif /* end of FZ_HASH_H */









