/**
 * @file printf.c
 * @author Craig Edwards
 * @copyright 2012–2025
 *
 * @brief Minimal printf implementation for Retro Rocket
 *
 * @section provenance Provenance
 * This implementation originated from a public-domain printf clone
 * commonly circulated in early OSDev / hobbyist C runtimes. The original
 * code dates back to the 1990s, likely adapted from 16‑bit DOS/Minix
 * environments where:
 *   - `PR_16` meant "use 16‑bit short",
 *   - `PR_32` meant "use 32‑bit long",
 *   - `PR_FP` supported "far pointers",
 * and short identifiers were required because some compilers only
 * guaranteed 8‑character identifier uniqueness.
 *
 * This version has since been heavily modernised:
 *   - All 16‑bit and far pointer cruft removed.
 *   - Flags renamed for clarity (`PR_INT32`, `PR_INT64`, `PR_WAS_NEG`).
 *   - Goto-based control flow replaced with structured `switch`/`if`.
 *   - Doxygen-style documentation added.
 *   - Confirmed fit for 32‑/64‑bit protected mode systems.
 *
 * While the skeleton came from public domain code, the current
 * implementation is effectively a clean-room rework and is considered
 * part of the Retro Rocket codebase.
 *
 * @section limitations Limitations
 * - Floating point formats (%f, %e, %g) are not implemented.
 * - Width/precision parsing is minimal.
 * - Thread safety relies on external spinlocks around vprintf/dprintf.
 *
 * @section intent Intent
 * The purpose of this implementation is to provide a lightweight,
 * dependency-free printf family for kernel and low-level userland code,
 * suitable for use in kernel code where full libc is not available.
 */

#pragma once

#include "kernel.h"

/**
 * @brief Variadic argument support
 * GCC/Clang builtins are used for variadic argument handling,
 * ensuring this remains freestanding without requiring stdarg.h.
 */
typedef __builtin_va_list va_list;
#define va_start(ap, X) __builtin_va_start(ap, X)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

/**
 * @brief Format Flags
 *
 * These flags are set during format string parsing and affect the
 * behaviour of string/number emission. They may be combined.
 */
#define PR_LEFT_JUSTIFY	0x001    /**< Left justify within field width (%-d). */
#define PR_UPPER_HEX	0x002    /**< Use A–F instead of a–f for hex digits (%X). */
#define PR_SIGNED	0x004    /**< Signed numeric conversion (%d vs. %u). */
#define PR_INT64   	0x008    /**< Treat numeric as 32-bit long. */
#define PR_INT32	0x010    /**< Treat numeric as 16-bit short. */
#define PR_WAS_NEG	0x020    /**< Write sign ('-') because PR_SG was set and number < 0. */
#define PR_LEFT_ZEROES	0x040    /**< Pad left with zeroes instead of spaces. */

/**
 * @brief Maximum temporary buffer length for number-to-string conversion.
 *
 * Largest number handled is 2^64–1. In base 8, this is 21 digits.
 * Add 1 for sign, 1 for NUL terminator, and a few for slop.
 */
#define PR_BUFLEN 26

/**
 * @brief Output Function Type
 *
 * Each printf variant passes characters to a callback function
 * of this type. This decouples formatting from the output device.
 *
 * @param c       Character to output
 * @param helper  Opaque pointer (e.g. buffer write position)
 * @param end     End-of-buffer guard (if bounded)
 * @return        0 on success, nonzero if buffer/device is full
 */
typedef int (*fnptr_t)(unsigned c, void **helper, const void* end);

/**
 * @brief Write formatted output to a string buffer with max size.
 * @param buf Output buffer
 * @param max Maximum number of characters (including NUL terminator)
 * @param fmt Format string
 * @param args Variadic argument list
 * @return Number of characters written (excluding NUL terminator)
 */
int vsnprintf(char *buf, size_t max, const char *fmt, va_list args);

/**
 * @brief Write formatted output to a string buffer with max size.
 * @param buf Output buffer
 * @param max Maximum number of characters (including NUL terminator)
 * @param fmt Format string
 * @param ... Format arguments
 * @return Number of characters written (excluding NUL terminator)
 */
int snprintf(char *buf, size_t max, const char *fmt, ...);

/**
 * @brief Write formatted output to the active console.
 *        Thread-safe via spinlocks.
 * @param fmt Format string
 * @param args Variadic argument list
 * @return Number of characters written
 */
int vprintf(const char *fmt, va_list args);

/**
 * @brief Write formatted output to the active console.
 *        Thread-safe via spinlocks.
 * @param fmt Format string
 * @param ... Format arguments
 * @return Number of characters written
 */
int printf(const char *fmt, ...);

/**
 * @brief Debug output only (I/O port 0xE9).
 *
 * Characters are written to the Bochs/QEMU debug console,
 * prefixed with a timestamp (kernel tick count).
 *
 * @param fmt Format string
 * @param ... Format arguments
 * @return Number of characters written
 */
int dprintf(const char *fmt, ...);
