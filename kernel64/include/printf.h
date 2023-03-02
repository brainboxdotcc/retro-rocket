#ifndef __PRINTF__H__
#define __PRINTF__H__

typedef __builtin_va_list va_list;
#define va_start(ap, X) __builtin_va_start(ap, X)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

/* flags used in processing format string */
#define PR_LJ   0x01    /* left justify */
#define PR_CA   0x02    /* use A-F instead of a-f for hex */
#define PR_SG   0x04    /* signed numeric conversion (%d vs. %u) */
#define PR_32   0x08    /* long (32-bit) numeric conversion */
#define PR_16   0x10    /* short (16-bit) numeric conversion */
#define PR_WS   0x20    /* PR_SG set and num was < 0 */
#define PR_LZ   0x40    /* pad left with '0' instead of ' ' */
#define PR_FP   0x80    /* pointers are far */
/* largest number handled is 2^32-1, lowest radix handled is 8.
2^32-1 in base 8 has 11 digits (add 5 for trailing NUL and for slop) */
#define PR_BUFLEN       16

typedef int (*fnptr_t)(unsigned c, void **helper);

int vsprintf(char *buf, const char *fmt, va_list args);

int sprintf(char *buf, const char *fmt, ...);

int vprintf(const char *fmt, va_list args);

int printf(const char *fmt, ...);

#endif
