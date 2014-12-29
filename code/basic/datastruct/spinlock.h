/* 
 * $Id: fo_spinlock.h,v 1.0, 07/06/2014 23:23:35 Exp$ 
 *
 * Copyright (c) 2014 Fortinet, Inc. All rights reserved.
 * Written by Andrew Wang <wywang@fortinet.com>
 *  
 */
#ifndef SPINLOCK_H
#define SPINLOCK_H

/**
 * The fo_spinlock_t type.
 */
typedef struct {
	volatile int32_t locked; /**< lock status 0 = unlocked, 1 = locked */
} fo_spinlock_t;

/**
 * A static spinlock initializer.
 */
#define FO_SPINLOCK_INITIALIZER { 0 }

/**
 * Initialize the spinlock to an unlocked state.
 *
 * @param sl
 *   A pointer to the spinlock.
 */
static inline void
fo_spinlock_init(fo_spinlock_t *sl)
{
	sl->locked = 0;
}

/**
 * Take the spinlock.
 *
 * @param sl
 *   A pointer to the spinlock.
 */
static inline void
fo_spinlock_lock(fo_spinlock_t *sl)
{
	int32_t lock_val = 1;
	asm volatile (
			"1:\n"
			"xchg %[locked], %[lv]\n"
			"test %[lv], %[lv]\n"
			"jz 3f\n"
			"2:\n"
			"pause\n"
			"cmpl $0, %[locked]\n"
			"jnz 2b\n"
			"jmp 1b\n"
			"3:\n"
			: [locked] "=m" (sl->locked), [lv] "=q" (lock_val)
			: "[lv]" (lock_val)
			: "memory");
}

/**
 * Release the spinlock.
 *
 * @param sl
 *   A pointer to the spinlock.
 */
static inline void
fo_spinlock_unlock (fo_spinlock_t *sl)
{
	int32_t unlock_val = 0;
	asm volatile (
			"xchg %[locked], %[ulv]\n"
			: [locked] "=m" (sl->locked), [ulv] "=q" (unlock_val)
			: "[ulv]" (unlock_val)
			: "memory");
}

/**
 * Try to take the lock.
 *
 * @param sl
 *   A pointer to the spinlock.
 * @return
 *   1 if the lock is successfully taken; 0 otherwise.
 */
static inline int
fo_spinlock_trylock (fo_spinlock_t *sl)
{
	int32_t lockval = 1;

	asm volatile (
			"xchg %[locked], %[lockval]"
			: [locked] "=m" (sl->locked), [lockval] "=q" (lockval)
			: "[lockval]" (lockval)
			: "memory");

	return (lockval == 0);
}

/**
 * Test if the lock is taken.
 *
 * @param sl
 *   A pointer to the spinlock.
 * @return
 *   1 if the lock is currently taken; 0 otherwise.
 */
static inline int32_t fo_spinlock_is_locked (fo_spinlock_t *sl)
{
	return sl->locked;
}
#endif /* !__FO_SPINLOCK_H__ */

