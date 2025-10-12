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

#include <io.h>
#include <stdint.h>
#include <stdbool.h>
#include <xmmintrin.h>


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

/**
 * @brief Read the current RFLAGS register.
 *
 * Captures the full 64-bit RFLAGS value from the CPU.
 *
 * @return The current value of the RFLAGS register.
 */
static inline uint64_t read_rflags(void) {
	uint64_t flags;
	__asm__ volatile("pushfq; pop %0" : "=r"(flags));
	return flags;
}

/**
 * @brief Write a value into the RFLAGS register.
 *
 * Restores the CPU's RFLAGS register to the specified value.
 * Typically used to restore saved interrupt flag state.
 *
 * @param flags The 64-bit value to load into RFLAGS.
 */
static inline void write_rflags(uint64_t flags) {
	__asm__ volatile("push %0; popfq" :: "r"(flags) : "memory", "cc");
}

/**
 * @brief Query whether interrupts are currently enabled.
 *
 * Uses the IF (Interrupt Flag) in the RFLAGS register to determine
 * if maskable interrupts are permitted.
 *
 * @return true if interrupts are enabled (IF = 1), false otherwise.
 */
static inline bool are_interrupts_enabled(void) {
	return (read_rflags() & (1ULL << 9)) != 0;
}

/**
 * @brief Attempt to acquire a spinlock once (non-blocking).
 *
 * Performs an atomic compare-and-swap on the lock.
 *
 * @param lock Pointer to the spinlock variable.
 * @return true if the lock was acquired, false otherwise.
 */
static inline bool try_lock(spinlock_t *lock) {
	uint32_t expected = 0;
	return __atomic_compare_exchange_n(lock, &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

/**
 * @brief Busy-wait until a spinlock is acquired.
 *
 * Continuously calls try_lock() until success.
 * Uses the `pause` instruction to reduce contention on hyper-threaded CPUs.
 *
 * @param lock Pointer to the spinlock variable.
 */
static inline void spin_until_locked(spinlock_t *lock) {
	while (!try_lock(lock)) {
		__builtin_ia32_pause(); // reduce contention
	}
}

/**
 * @brief Acquire a spinlock while disabling interrupts.
 *
 * Saves the current interrupt flag (IF) state into @p flags,
 * disables interrupts, then spins until the lock is acquired.
 *
 * @param lock  Pointer to the spinlock variable.
 * @param flags Pointer to a uint64_t used to store the saved RFLAGS register.
 */
static inline void lock_spinlock_irq(spinlock_t *lock, uint64_t *flags) {
	*flags = read_rflags();   // save current IF
	interrupts_off();         // mask interrupts
	spin_until_locked(lock);  // acquire
}

/**
 * @brief Release a spinlock and restore interrupts.
 *
 * Releases the given spinlock and restores the CPU interrupt flag (IF)
 * from the previously saved @p flags value.
 *
 * @param lock  Pointer to the spinlock variable.
 * @param flags RFLAGS value saved by lock_spinlock_irq().
 */
static inline void unlock_spinlock_irq(spinlock_t *lock, uint64_t flags) {
	__atomic_store_n(lock, 0, __ATOMIC_RELEASE); // unlock
	write_rflags(flags);                         // restore IF exactly as before
}
