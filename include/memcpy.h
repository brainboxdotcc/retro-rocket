/**
 * @file memcpy.h
 * @brief Memory manipulation helper functions.
 *
 * Provides standard routines for copying, moving, comparing,
 * setting, and reversing blocks of memory. These are implemented
 * as part of the kernel runtime and are not dependent on libc.
 *
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2026
 */
#pragma once

#include "kernel.h"

/* ------------------------------------------------------------------------- */
/* Standard memory manipulation functions                                    */
/* ------------------------------------------------------------------------- */

/**
 * @brief Copy a block of memory.
 *
 * Copies @p len bytes from the buffer @p src to the buffer @p dest.
 * Behaviour is undefined if the memory regions overlap.
 *
 * @param dest Destination buffer.
 * @param src Source buffer.
 * @param len Number of bytes to copy.
 * @return Pointer to @p dest.
 */
void* memcpy(void* dest, const void* src, size_t len);

/**
 * @brief Move a block of memory.
 *
 * Copies @p n bytes from the buffer @p src to the buffer @p dest.
 * Unlike memcpy(), handles overlapping regions safely.
 *
 * @param dest Destination buffer.
 * @param src Source buffer.
 * @param n Number of bytes to copy.
 * @return Pointer to @p dest.
 */
void* memmove(void* dest, const void* src, size_t n);

/**
 * @brief Compare two memory blocks.
 *
 * Compares @p n bytes of the buffers @p s1 and @p s2.
 *
 * @param s1 First buffer.
 * @param s2 Second buffer.
 * @param n Number of bytes to compare.
 * @return An integer less than, equal to, or greater than zero if the first
 *         differing byte in @p s1 is found to be less than, equal to, or greater
 *         than the corresponding byte in @p s2. Returns zero if all compared
 *         bytes are equal.
 */
int memcmp(const void* s1, const void* s2, size_t n);

/**
 * @brief Fill a block of memory with a constant value.
 *
 * Sets the first @p len bytes of the buffer @p dest to the value @p val.
 *
 * This implementation is performed using inline assembly (`rep stosb`) and is
 * not replaced by compiler builtins. The write is always emitted as an explicit
 * memory operation and will not be optimised away as a dead store.
 *
 * This makes it suitable for deterministic memory clearing in low-level code.
 * However, it does not guarantee complete removal of sensitive data from all
 * intermediate locations (e.g. registers or temporary copies).
 *
 * @param dest Destination buffer.
 * @param val Value to set.
 * @param len Number of bytes to set.
 * @return Pointer to @p dest.
 */
void* memset(void* dest, char val, size_t len);

/**
 * @brief Zero a block of memory.
 *
 * Sets the first @p len bytes of the buffer @p dest to zero.
 *
 * This is a thin wrapper around memset(). In Retro Rocket, memset is
 * implemented in assembly and is not elided or replaced by the compiler,
 * ensuring the write is always performed.
 *
 * @param dest Destination buffer.
 * @param len Number of bytes to clear.
 */
static inline __attribute__((always_inline)) void memzero(void* dest, size_t len) {
	memset(dest, 0, len);
}

/* ------------------------------------------------------------------------- */
/* Additional helpers                                                         */
/* ------------------------------------------------------------------------- */

/**
 * @brief Reverse a block of memory in place.
 *
 * Reverses the order of @p n bytes in the buffer @p buf.
 *
 * @param buf Pointer to the buffer to reverse.
 * @param n Number of bytes to reverse.
 * @return Number of bytes reversed (same as @p n).
 */
size_t memrev(char* buf, size_t n);
