#ifndef __PRINTF__H__
#define __PRINTF__H__

/* Assume: width of stack == width of int. Don't use sizeof(char *) or
other pointer because sizeof(char *)==4 for LARGE-model 16-bit code.
Assume: width is a power of 2 */
#define STACK_WIDTH     sizeof(int)

/* Round up object width so it's an even multiple of STACK_WIDTH.
Using & for division here, so STACK_WIDTH must be a power of 2. */
#define TYPE_WIDTH(TYPE)                                \
        ((sizeof(TYPE) + STACK_WIDTH - 1) & ~(STACK_WIDTH - 1))

/* point the va_list pointer to LASTARG,
then advance beyond it to the first variable arg */
#define va_start(PTR, LASTARG)                          \
        PTR = (va_list)((char *)&(LASTARG) + TYPE_WIDTH(LASTARG))

#define va_end(PTR)     /* nothing */

/* Increment the va_list pointer, then return
(evaluate to, actually) the previous value of the pointer.
WHEEE! At last; a valid use for the C comma operator! */
#define va_arg(PTR, TYPE)       (                       \
        *((TYPE *)((char *)(PTR) - TYPE_WIDTH(TYPE)))   \
                                )

#define va_incr(PTR, TYPE) PTR += TYPE_WIDTH(TYPE)
/* Every other compiler/libc seems to be using 'void *', so...
(I _was_ using 'unsigned char *') */
typedef void *va_list;

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
