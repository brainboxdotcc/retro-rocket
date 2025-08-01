/**
 * @file spinlock.h
 * @author Craig Edwards
 * @brief Simple x86 spinlock implementation for Retro Rocket kernel.
 *
 * Provides lightweight synchronisation primitives for protecting short critical
 * sections between multiple CPUs or interrupt contexts. Designed for cases where
 * blocking or sleeping is not possible.
 *
 * @note Spinlocks are busy-waiting and should only be used to protect fast
 *       operations (e.g. allocator, PID table, queue updates). Long critical
 *       sections will waste CPU cycles.
 *
 * @copyright Copyright (c) 2012-2025
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @typedef spinlock_t
 * @brief Opaque 32-bit integer used as a spinlock state.
 *
 * A value of 0 indicates the lock is free; non-zero indicates held.
 */
typedef uint32_t spinlock_t;

/**
 * @brief Initialise a spinlock to the unlocked state.
 *
 * @param lock Pointer to the spinlock to initialise.
 */
static inline void init_spinlock(spinlock_t* lock) {
	__atomic_store_n(lock, 0, __ATOMIC_RELAXED);
}

/**
 * @brief Acquire a spinlock.
 *
 * Repeatedly tests and sets the lock until it becomes available.
 * Uses x86 `pause` instruction to reduce power and bus contention
 * while spinning.
 *
 * @param lock Pointer to the spinlock to acquire.
 *
 * @note This function does not return until the lock is held.
 */
static inline void lock_spinlock(spinlock_t* lock) {
	while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE)) {
		while (__atomic_load_n(lock, __ATOMIC_RELAXED)) {
			__builtin_ia32_pause(); // x86 "pause"
		}
	}
}

/**
 * @brief Release a spinlock.
 *
 * @param lock Pointer to the spinlock to release.
 */
static inline void unlock_spinlock(spinlock_t* lock) {
	__atomic_clear(lock, __ATOMIC_RELEASE);
}

/**
 * @brief Attempt to acquire a spinlock without blocking.
 *
 * @param lock Pointer to the spinlock to try and acquire.
 * @return true if the lock was successfully acquired, false if it was already held.
 *
 * @note Useful in schedulers or other contexts where blocking indefinitely
 *       is undesirable.
 */
static inline bool try_lock_spinlock(spinlock_t* lock) {
	return !__atomic_test_and_set(lock, __ATOMIC_ACQUIRE);
}
