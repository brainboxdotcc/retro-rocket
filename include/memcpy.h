/**
 * @file memcpy.h
 * @brief Memory manipulation helper functions.
 *
 * Provides standard routines for copying, moving, comparing,
 * setting, and reversing blocks of memory. These are implemented
 * as part of the kernel runtime and are not dependent on libc.
 *
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
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
void* memcpy(void* dest, const void* src, uint64_t len);

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
void* memmove(void* dest, const void* src, uint64_t n);

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
int memcmp(const void* s1, const void* s2, uint64_t n);

/**
 * @brief Fill a block of memory with a constant value.
 *
 * Sets the first @p len bytes of the buffer @p dest to the value @p val.
 *
 * @param dest Destination buffer.
 * @param val Value to set.
 * @param len Number of bytes to set.
 * @return Pointer to @p dest.
 */
void* memset(void* dest, char val, uint64_t len);

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
