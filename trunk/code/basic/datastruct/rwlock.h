/** 
 *	@file	rwlock.h 
 *	
 *	@brief	using ASM language implement rwlock.
 *
 *	@author
 */
#ifndef RWLOCK_H
#define RWLOCK_H

#include <sys/types.h>

#define MPLOCK		"lock ; "       /* Insert MP lock prefix */

typedef struct rwlock {
	volatile int	cnt; /* -1 when wrlock held, > 0 when rdlocks held */
} rwlock_t;

#define	RWLOCK_DEFINE(name)	\
	rwlock_t	name = {0}

/**
 * Atomic compare and set.
 *
 * (atomic) equivalent to:
 *   if (*dst == exp)
 *     *dst = src (all 32-bit words)
 *
 * @param dst
 *   The destination location into which the value will be written.
 * @param exp
 *   The expected value.
 * @param src
 *   The new value.
 * @return
 *   Non-zero on success; 0 on failure.
 */
static inline u_int32_t
atomic32_cmpset(volatile u_int32_t *dst, u_int32_t exp, u_int32_t src)
{
	u_int8_t res;

	asm volatile(
			MPLOCKED
			"cmpxchgl %[src], %[dst];"
			"sete %[res];"
			: [res] "=a" (res),     /* output */
			  [dst] "=m" (*dst)
			: [src] "r" (src),      /* input */
			  "a" (exp),
			  "m" (*dst)
			: "memory");            /* no-clobber list */
	return res;
}

/**
 * Atomically increment a counter by one.
 *
 * @param v
 *   A pointer to the atomic counter.
 */
static inline void
atomic32_inc(volatile int32_t *cnt)
{
	asm volatile(
			MPLOCKED
			"incl %[cnt]"
			: [cnt] "=m" (*cnt)   /* output */
			: "m" (*cnt)          /* input */
		    );
}

/**
 * Atomically decrement a counter by one.
 *
 * @param v
 *   A pointer to the atomic counter.
 */
static inline void
atomic32_dec(volatile int32_t *cnt)
{
	asm volatile(
			MPLOCKED
			"decl %[cnt]"
			: [cnt] "=m" (*cnt)   /* output */
			: "m" (*cnt)          /* input */
		    );
}

/**
 * Initialize the rwlock to an unlocked state.
 *
 * @param rwl
 *   A pointer to the rwlock structure.
 */
static inline void
rwlock_init(fo_rwlock_t *rwl)
{
	rwl->cnt = 0;
}

/**
 * Take a read lock. Loop until the lock is held.
 *
 * @param rwl
 *   A pointer to a rwlock structure.
 */
static inline void
rwlock_rdlock(rwlock_t *rwl)
{
	int32_t x;
	int success = 0;

	while (success == 0) {
		x = rwl->cnt;
		/* write lock is held */
		if (x < 0) {
			CPU_PAUSE();
			continue;
		}
		success = atomic32_cmpset((volatile u_int32_t *)&rwl->cnt,
					      x, x + 1);
	}
}

/**
 * Release a read lock.
 *
 * @param rwl
 *   A pointer to the rwlock structure.
 */
static inline void
rwlock_rdunlock(rwlock_t *rwl)
{
	atomic32_dec(&rwl->cnt);
}

/**
 * Take a write lock. Loop until the lock is held.
 *
 * @param rwl
 *   A pointer to a rwlock structure.
 */
static inline void
rwlock_wrlock(rwlock_t *rwl)
{
	int32_t x;
	int success = 0;

	while (success == 0) {
		x = rwl->cnt;
		/* a lock is held */
		if (x != 0) {
			CPU_PAUSE();
			continue;
		}
		success = atomic32_cmpset((volatile u_int32_t *)&rwl->cnt,
					      0, -1);
	}
}

/**
 * Release a write lock.
 *
 * @param rwl
 *   A pointer to a rwlock structure.
 */
static inline void
rwlock_wrunlock(fo_rwlock_t *rwl)
{
	atomic32_inc(&rwl->cnt);
}


#endif /* RWLOCK_H */

