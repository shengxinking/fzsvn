/**
 *	@file	hash.h
 *	@brief	A simple hash implement.
 *
 *	@author	Forrest.zhang
 */

#ifndef FZ_HASH_H
#define FZ_HASH_H

#include <sys/types.h>

#include "cblist.h"
#include "objpool.h"

typedef int  (*hash_cmp_func)(const void *val1, const void *val2);
typedef void (*hash_free_func)(void *val);
typedef void (*hash_print_func)(const void *val);

typedef struct hash_item {
	cblist_t	list;		/* in hash list */
	void		*data;
} hash_item_t;

typedef struct hash_entry {
	cblist_t	list;		/* list header */
	u_int32_t	size;		/* number items in entry */
} hash_entry_t;

typedef struct hash_stat {
	int		nconflict;	/* conlict times */
	int		nfind_failed;	/* find failed times */
	int		nfind_success;	/* find success times */
} hash_stat_t;

typedef struct hash {
	objpool_t	*pool;		/* hash_item pool */
	hash_entry_t    *entries;	/* hash entries */
	u_int32_t	nentry;		/* number of entries */
	hash_cmp_func   cmp_func;	/* item compare function */
	u_int32_t	size;		/* item number in hash */
	hash_stat_t	stat;		/* hash statisitic data */
	u_int16_t	need_lock:1;	/* need lock or not for thread-safe */ 
	pthread_mutex_t	lock;		/* lock for thread-safe */
} hash_t;

/**
 *	Alloc a new hash object and return it. the hash size is @size, 
 *	Object compare function is @cmp_func.
 *
 *	Return hash object if success, NULL on error.
 */
extern hash_t *
hash_alloc(u_int32_t size, hash_cmp_func cmp_func, int locked);

/**
 *	Free a hash object @hash alloced by hash_alloc.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
hash_free(hash_t *hash, hash_free_func free_func);

/**
 *	Find a object in hash @hash.
 *
 *	Return object if found, NULL not founded.
 */
extern void * 
hash_search(hash_t *hash, u_int32_t key, const void *val);

/**
 *	Insert a object @data into hash @hash. the hash key
 *	is @key. It not check duplicated element in hash.
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
hash_insert(hash_t *hash, u_int32_t key, void *data);

/**
 *	Insert a object @data into hash @hash. the hash key
 *	is @key. It'll check duplicated element in hash.
 *
 *	Return 0 if success, -1 on error or duplicated item
 *	founded.
 */
extern int 
hash_insert_nodup(hash_t *hash, u_int32_t key, void *data);

/**
 *	Delete a object @data from hash @hash. the hash key
 *	is @key.
 *
 *	Return the hash data if success, NULL if failed.
 */
extern void * 
hash_delete(hash_t *hash, u_int32_t key, const void *var);

/**
 *	Return the hash object number in hash @hash.
 *
 *	Return >= 0 if success, -1 on error.
 */
extern int 
hash_size(const hash_t *hash);

/**
 *	Print all elements in hash @hash. if the object
 *	printf funtion @print_func is not NULL, also print
 *	the object content
 *
 *	Return 0 if success, -1 on error.
 */
extern int 
hash_print(const hash_t *hash, hash_print_func print_func);


#endif /* end of FZ_HASH_H */

