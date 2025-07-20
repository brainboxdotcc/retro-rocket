/**
 * @file spinlock.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2023
 */
#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

typedef uint32_t spinlock;

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t spinlock_t;

static inline void init_spinlock(spinlock_t* lock) {
	__atomic_store_n(lock, 0, __ATOMIC_RELAXED);
}

static inline void lock_spinlock(spinlock_t* lock) {
	while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE)) {
		while (__atomic_load_n(lock, __ATOMIC_RELAXED)) {
			__builtin_ia32_pause(); // x86 "pause"
		}
	}
}

static inline void unlock_spinlock(spinlock_t* lock) {
	__atomic_clear(lock, __ATOMIC_RELEASE);
}



#endif

