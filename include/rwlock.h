/**
 * @file rwlock.h
 * @author Craig Edwards
 * @brief Simple readers–writer lock implementation for SMP safety
 * @copyright Copyright (c) 2012-2025
 *
 * Readers–writer locks allow multiple concurrent readers, but
 * exclusive access for a single writer.
 *
 * Implementation notes:
 *  - Positive values of `state` = number of active readers
 *  - Zero = lock is free
 *  - -1 = held by a writer
 *
 * Memory ordering:
 *  - Readers/writers use acquire semantics on lock
 *  - Writers release when unlocking
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <x86intrin.h>

typedef struct {
	int32_t state;
} rwlock_t;

/**
 * @brief Initialise an RW lock to unlocked state
 */
static inline void init_rwlock(rwlock_t *lock) {
	__atomic_store_n(&lock->state, 0, __ATOMIC_RELAXED);
}

/**
 * @brief Acquire the lock for reading.
 * Multiple readers may hold the lock simultaneously.
 */
static inline void rwlock_read_lock(rwlock_t *lock) {
	for (;;) {
		int s = __atomic_load_n(&lock->state, __ATOMIC_RELAXED);
		if (s >= 0) {
			if (__atomic_compare_exchange_n(&lock->state, &s, s + 1,
							true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
				return;
			}
		}
		__builtin_ia32_pause(); // x86 "pause"
	}
}

/**
 * @brief Release the lock after reading.
 */
static inline void rwlock_read_unlock(rwlock_t *lock) {
	__atomic_fetch_sub(&lock->state, 1, __ATOMIC_RELEASE);
}

/**
 * @brief Acquire the lock for writing.
 * Blocks until all readers have finished and no other writer holds the lock.
 */
static inline void rwlock_write_lock(rwlock_t *lock) {
	int expected = 0;
	while (!__atomic_compare_exchange_n(&lock->state, &expected, -1,
					    true, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
		expected = 0;
		__builtin_ia32_pause();
	}
}

/**
 * @brief Release the lock after writing.
 */
static inline void rwlock_write_unlock(rwlock_t *lock) {
	__atomic_store_n(&lock->state, 0, __ATOMIC_RELEASE);
}
