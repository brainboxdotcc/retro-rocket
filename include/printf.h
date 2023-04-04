#pragma once

#include "kernel.h"

typedef __builtin_va_list va_list;
#define va_start(ap, X) __builtin_va_start(ap, X)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

/* flags used in processing format string */
#define PR_LJ   0x001    /* left justify */
#define PR_CA   0x002    /* use A-F instead of a-f for hex */
#define PR_SG   0x004    /* signed numeric conversion (%d vs. %u) */
#define PR_32   0x008    /* long (32-bit) numeric conversion */
#define PR_16   0x010    /* short (16-bit) numeric conversion */
#define PR_WS   0x020    /* PR_SG set and num was < 0 */
#define PR_LZ   0x040    /* pad left with '0' instead of ' ' */
#define PR_FP   0x080    /* pointers are far */
#define PR_64   0x100    /* long (32-bit) numeric conversion */
/* largest number handled is 2^64-1, lowest radix handled is 8.
2^64-1 in base 8 has 21 digits (add 5 for trailing NUL and for slop) */
#define PR_BUFLEN       26

typedef int (*fnptr_t)(unsigned c, void **helper, const void* end);

int vsprintf(char *buf, const char *fmt, va_list args);

int vsnprintf(char *buf, size_t max, const char *fmt, va_list args);

int sprintf(char *buf, const char *fmt, ...);

int snprintf(char *buf, size_t max, const char *fmt, ...);

int vprintf(const char *fmt, va_list args);

int printf(const char *fmt, ...);

/**
 * @brief Debug only to int 0xE9
 * 
 * @param fmt format specifier
 * @param ... format args
 * @return int number of printed characters
 */
int dprintf(const char *fmt, ...);
