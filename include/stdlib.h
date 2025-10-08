/**
 * @file stdlib.h
 * @author Craig Edwards
 * @brief Minimal standard library stubs for Retro Rocket
 * @date 2023-05-27
 * @copyright Copyright (c) 2023
 *
 * Provides a reduced subset of C standard library functions.
 * Many functions are provided only as stubs for compatibility.
 * Multibyte and wide-character conversions are not supported.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifndef __WCTYPE_T_DEFINED
typedef int wctype_t;
#endif

#ifndef __MBSTATE_T_DEFINED
typedef struct {
	unsigned int __dummy;
} mbstate_t;
#endif

/**
 * @brief Result of integer division
 */
typedef struct div_t {
	int quot; /**< Quotient */
	int rem;  /**< Remainder */
} div_t;

/**
 * @brief Result of long integer division
 */
typedef struct ldiv_t {
	long int quot; /**< Quotient */
	long int rem;  /**< Remainder */
} ldiv_t;

typedef int wctype_t;

enum {
	_WC_ALPHA=1, _WC_DIGIT, _WC_ALNUM, _WC_SPACE, _WC_UPPER, _WC_LOWER,
	_WC_XDIGIT, _WC_PUNCT, _WC_CNTRL, _WC_GRAPH, _WC_PRINT, _WC_BLANK
};

typedef int wint_t;

/**
 * @brief Alias type for signed char (used in stubs)
 */
typedef char schar_t;

/**
 * @brief Convert string to double
 *
 * @param str Input string
 * @param endptr Pointer to store end of parsed string
 * @return double Converted value
 */
double strtod(const char *str, char **endptr);

/**
 * @brief Convert string to long integer
 *
 * @param str Input string
 * @param endptr Pointer to store end of parsed string
 * @param base Numerical base (e.g. 10, 16)
 * @return long int Converted value
 */
long int strtol(const char *str, char **endptr, int base);

/**
 * @brief Convert string to unsigned long integer
 *
 * @param str Input string
 * @param endptr Pointer to store end of parsed string
 * @param base Numerical base (e.g. 10, 16)
 * @return unsigned long int Converted value
 */
unsigned long int strtoul(const char *str, char **endptr, int base);

/**
 * @brief Terminate the program abnormally
 */
void abort(void);

/**
 * @brief Register a function to be called at program exit
 *
 * @note Stub only. Retro Rocket does not have a concept of process
 *       termination for C programs, so this function does nothing.
 *
 * @param func Exit handler (ignored)
 * @return int Always returns 0
 */
int atexit(void (*func)(void));

/**
 * @brief Terminate the program with status
 *
 * @note Stub only. Retro Rocket does not execute standalone C programs,
 *       so this function does nothing.
 *
 * @param status Exit code (ignored)
 */
void exit(int status);

/**
 * @brief Get the value of an environment variable
 *
 * @note Stub only. Retro Rocket is a non-POSIX system with no
 *       environment variables. Always returns NULL.
 *
 * @param name Name of the environment variable (ignored)
 * @return char* Always NULL
 */
char *getenv(const char *name);

/**
 * @brief Execute a shell command
 *
 * @note Stub only. Retro Rocket cannot execute native programs.
 *       Always returns -1.
 *
 * @param string Command string (ignored)
 * @return int Always -1
 */
int system(const char *string);


/**
 * @brief Binary search
 *
 * @param key Key to search for
 * @param base Array base pointer
 * @param nitems Number of items in array
 * @param size Size of each element
 * @param compar Comparison function
 * @return void* Pointer to found element or NULL
 */
void *bsearch(const void *key, const void *base, size_t nitems, size_t size,
	      int (*compar)(const void *, const void *));

/**
 * @brief Quicksort
 *
 * @param base Array base pointer
 * @param nitems Number of items
 * @param size Size of each element
 * @param compar Comparison function
 */
void qsort(void *base, size_t nitems, size_t size,
	   int (*compar)(const void *, const void*));

/**
 * @brief Divide two integers
 *
 * @param numer Numerator
 * @param denom Denominator
 * @return div_t Quotient and remainder
 */
div_t div(int numer, int denom);

/**
 * @brief Divide two long integers
 *
 * @param numer Numerator
 * @param denom Denominator
 * @return ldiv_t Quotient and remainder
 */
ldiv_t ldiv(long int numer, long int denom);

/**
 * @brief Generate a pseudo-random number
 *
 * @return int Pseudo-random number
 */
int rand(void);

/**
 * @brief Seed the random number generator
 *
 * @param seed Seed value
 */
void srand(unsigned int seed);

/**
 * @brief Determine the number of bytes in the next multibyte character.
 *
 * Minimal implementation: always returns 0 or 1 depending on whether *str is
 * '\0'. Assumes single-byte encoding.
 *
 * @param str Pointer to the multibyte string to examine.
 * @param n Maximum number of bytes to inspect.
 * @return 0 if str is NULL or points to '\0', otherwise 1.
 */
int mblen(const char *str, size_t n);

/**
 * @brief Convert a multibyte string to a wide-character string.
 *
 * Minimal implementation: treats each byte as one wide character.
 *
 * @param pwcs Destination buffer for wide characters (may be NULL).
 * @param str Source multibyte string.
 * @param n Maximum number of wide characters to write.
 * @return Number of wide characters written (excluding terminator).
 */
size_t mbstowcs(wchar_t *pwcs, const char *str, size_t n);

/**
 * @brief Convert the next multibyte character to a wide character.
 *
 * Minimal implementation: promotes the byte to wchar_t.
 *
 * @param pwc Destination wide character (may be NULL).
 * @param str Source multibyte character.
 * @param n Maximum number of bytes to inspect.
 * @return Number of bytes consumed (0 if str is NULL or empty).
 */
int mbtowc(wchar_t *pwc, const char *str, size_t n);

/**
 * @brief Convert a wide-character string to a multibyte string.
 *
 * Minimal implementation: truncates each wide character to the low 8 bits.
 *
 * @param str Destination buffer for multibyte string (may be NULL).
 * @param pwcs Source wide-character string.
 * @param n Maximum number of bytes to write.
 * @return Number of bytes written (excluding terminator).
 */
size_t wcstombs(char *str, const wchar_t *pwcs, size_t n);

/**
 * @brief Convert a wide character to a multibyte sequence.
 *
 * Minimal implementation: writes the low 8 bits as a single byte.
 *
 * @param str Destination buffer for multibyte sequence.
 * @param wchar Source wide character.
 * @return Number of bytes written (1), or 0 if str is NULL.
 */
int wctomb(char *str, wchar_t wchar);

/**
 * @brief Return the uppercase ASCII equivalent of a byte value.
 *
 * ASCII-only transformation. Non-letters are returned unchanged.
 *
 * @param c Input byte (0–255).
 * @return Uppercase form if 'a'..'z', otherwise @p c.
 */
int _uc(int c);

/**
 * @brief True if ASCII control character.
 *
 * Matches 0x00–0x1F and 0x7F only.
 *
 * @param c Input byte (0–255).
 * @return Non-zero if control, zero otherwise.
 */
int iscntrl(int c);

/**
 * @brief True if printable, non-space ASCII.
 *
 * Matches 0x21–0x7E.
 *
 * @param c Input byte (0–255).
 * @return Non-zero if graphic, zero otherwise.
 */
int isgraph(int c);

/**
 * @brief True if ASCII lowercase letter.
 *
 * Matches 'a'..'z'.
 *
 * @param c Input byte (0–255).
 * @return Non-zero if lowercase, zero otherwise.
 */
int islower(int c);

/**
 * @brief True if printable ASCII (space included).
 *
 * Matches 0x20–0x7E.
 *
 * @param c Input byte (0–255).
 * @return Non-zero if printable, zero otherwise.
 */
int isprint(int c);

/**
 * @brief True if ASCII punctuation.
 *
 * Printable, not alphanumeric, not space. ASCII-only.
 *
 * @param c Input byte (0–255).
 * @return Non-zero if punctuation, zero otherwise.
 */
int ispunct(int c);

/**
 * @brief Convert a C string to wide chars (ASCII pass-through).
 *
 * Copies up to @p n characters from @p src to @p dst, casting each byte
 * to @c wchar_t (0–255). Stops at NUL or when @p n is reached.
 *
 * @param dst Destination buffer (may be NULL to count only).
 * @param src Source NUL-terminated string.
 * @param n Maximum wide characters to write.
 * @return Number of wide characters produced (excluding terminator).
 */
size_t mbstowcs(wchar_t *dst, const char *src, size_t n);

/**
 * @brief Convert wide chars to a C string (ASCII pass-through).
 *
 * Copies up to @p n characters from @p src to @p dst, truncating each
 * @c wchar_t to a single byte. Stops at wide NUL or when @p n is reached.
 *
 * @param dst Destination buffer (may be NULL to count only).
 * @param src Source wide string.
 * @param n Maximum bytes to write.
 * @return Number of bytes produced (excluding terminator).
 */
size_t wcstombs(char *dst, const wchar_t *src, size_t n);

/**
 * @brief Convert the next multibyte character to wide (single-byte locale).
 *
 * ASCII-only, state-free. Treats each byte as one character.
 * On NUL, returns 0. @p ps is accepted but ignored.
 *
 * @param pwc Output wide character (may be NULL).
 * @param s Input byte buffer.
 * @param n Size of input buffer.
 * @param ps Conversion state (ignored).
 * @return 0 for NUL byte, 1 for a non-NUL byte, (size_t)-1 on error.
 */
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);

/**
 * @brief Convert one wide character to multibyte (single-byte locale).
 *
 * ASCII-only, state-free. Emits exactly one byte if @p wc <= 0xFF.
 * @p ps is accepted but ignored.
 *
 * @param s Destination buffer (must have space for 1 byte).
 * @param wc Input wide character.
 * @param ps Conversion state (ignored).
 * @return 1 on success, (size_t)-1 if @p wc > 0xFF.
 */
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps);

/**
 * @brief True if wide character is an ASCII letter.
 *
 * Matches 'A'..'Z' or 'a'..'z' (values within 0..255).
 *
 * @param wc Wide character.
 * @return Non-zero if alphabetic, zero otherwise.
 */
int iswalpha(wint_t wc);

/**
 * @brief True if wide character is ASCII alphanumeric.
 *
 * Matches letters or digits within 0..255.
 *
 * @param wc Wide character.
 * @return Non-zero if alphanumeric, zero otherwise.
 */
int iswalnum(wint_t wc);

/**
 * @brief True if wide character is ASCII blank.
 *
 * Matches space or tab only.
 *
 * @param wc Wide character.
 * @return Non-zero if blank, zero otherwise.
 */
int iswblank(wint_t wc);

/**
 * @brief True if wide character is ASCII control.
 *
 * @param wc Wide character.
 * @return Non-zero if control, zero otherwise.
 */
int iswcntrl(wint_t wc);

/**
 * @brief True if wide character is an ASCII digit.
 *
 * Matches '0'..'9'.
 *
 * @param wc Wide character.
 * @return Non-zero if digit, zero otherwise.
 */
int iswdigit(wint_t wc);

/**
 * @brief True if wide character is printable, non-space ASCII.
 *
 * Matches 0x21–0x7E.
 *
 * @param wc Wide character.
 * @return Non-zero if graphic, zero otherwise.
 */
int iswgraph(wint_t wc);

/**
 * @brief True if wide character is ASCII lowercase.
 *
 * Matches 'a'..'z'.
 *
 * @param wc Wide character.
 * @return Non-zero if lowercase, zero otherwise.
 */
int iswlower(wint_t wc);

/**
 * @brief True if wide character is printable ASCII.
 *
 * Matches 0x20–0x7E.
 *
 * @param wc Wide character.
 * @return Non-zero if printable, zero otherwise.
 */
int iswprint(wint_t wc);

/**
 * @brief True if wide character is ASCII punctuation.
 *
 * Printable, not alphanumeric, not space. ASCII-only.
 *
 * @param wc Wide character.
 * @return Non-zero if punctuation, zero otherwise.
 */
int iswpunct(wint_t wc);

/**
 * @brief True if wide character is ASCII whitespace.
 *
 * Matches space, tab, LF, CR, VT, FF.
 *
 * @param wc Wide character.
 * @return Non-zero if whitespace, zero otherwise.
 */
int iswspace(wint_t wc);

/**
 * @brief True if wide character is ASCII uppercase.
 *
 * Matches 'A'..'Z'.
 *
 * @param wc Wide character.
 * @return Non-zero if uppercase, zero otherwise.
 */
int iswupper(wint_t wc);

/**
 * @brief True if wide character is ASCII hexadecimal digit.
 *
 * Matches 0–9, A–F, a–f.
 *
 * @param wc Wide character.
 * @return Non-zero if hex digit, zero otherwise.
 */
int iswxdigit(wint_t wc);

/**
 * @brief Convert ASCII letter to uppercase.
 *
 * Non-letters returned unchanged. ASCII-only.
 *
 * @param wc Wide character.
 * @return Uppercase equivalent if 'a'..'z', otherwise @p wc.
 */
wint_t towupper(wint_t wc);

/**
 * @brief Convert ASCII letter to lowercase.
 *
 * Non-letters returned unchanged. ASCII-only.
 *
 * @param wc Wide character.
 * @return Lowercase equivalent if 'A'..'Z', otherwise @p wc.
 */
wint_t towlower(wint_t wc);

/**
 * @brief Test @p wc against a simple character class @p t.
 *
 * ASCII-only implementation backing TRE; supports a limited set
 * of class IDs as used by the regex engine.
 *
 * @param wc Wide character.
 * @param t Class identifier.
 * @return Non-zero if @p wc is in class @p t, zero otherwise.
 */
int iswctype(wint_t wc, wctype_t t);

/**
 * @brief Map a class name to a @c wctype_t identifier.
 *
 * Recognises a small ASCII subset used by TRE (e.g. "alpha",
 * "digit", "space", etc.). Returns 0 for unknown names.
 *
 * @param name NUL-terminated class name.
 * @return Class identifier, or 0 if not recognised.
 */
wctype_t wctype(const char *name);
