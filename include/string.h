/**
 * @file string.h
 * @brief Custom string handling functions with GC support for BASIC runtime
 * @author Craig Edwards
 * @copyright Copyright (c) 2012-2025
 */
#pragma once
#include "basic.h"

#define STRING_GC_AREA_SIZE (128 * 1024)

/**
 * @brief Linked list node for garbage-collected strings.
 *
 * Used internally by gc_strdup() and gc() to track
 * dynamically allocated strings for temporary BASIC execution.
 */
typedef struct gc_str_t {
	const char* ptr;           /**< Pointer to allocated string */
	struct gc_str_t* next;     /**< Next entry in linked list */
} gc_str_t;

/**
 * @brief Return the length of a string.
 *
 * @param str Null-terminated string
 * @return unsigned int Length of string
 */
unsigned int strlen(const char* str);

/**
 * @brief Compare two strings (case-sensitive).
 *
 * @param s1 First string
 * @param s2 Second string
 * @return int 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int strcmp(const char* s1, const char* s2);

/**
 * @brief Compare two strings (case-insensitive).
 *
 * @param s1 First string
 * @param s2 Second string
 * @return int 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int stricmp(const char* s1, const char* s2);

/**
 * @brief Compare first n characters of two strings (case-sensitive).
 *
 * @param s1 First string
 * @param s2 Second string
 * @param n Number of characters to compare
 * @return int Comparison result as with strcmp()
 */
int strncmp(const char* s1, const char* s2, uint32_t n);

/**
 * @brief Compare first n characters of two strings (case-insensitive).
 *
 * @param s1 First string
 * @param s2 Second string
 * @param n Number of characters to compare
 * @return int Comparison result as with strcmp()
 */
int strnicmp(const char* s1, const char* s2, uint32_t n);

/**
 * @brief Convert a character to uppercase.
 *
 * @param low Character to convert
 * @return char Uppercase equivalent if alphabetic, otherwise unchanged
 */
char toupper(char low);

/**
 * @brief Check if a character is uppercase.
 *
 * @param x Character to check
 * @return int Non-zero if uppercase, 0 otherwise
 */
int isupper(const char x);

/**
 * @brief Convert a character to lowercase.
 *
 * @param low Character to convert
 * @return char Lowercase equivalent if alphabetic, otherwise unchanged
 */
char tolower(char low);

/**
 * @brief Check if a character is alphanumeric.
 *
 * @param x Character to check
 * @return int Non-zero if alphanumeric, 0 otherwise
 */
int isalnum(const char x);

/**
 * @brief Check if a character is whitespace.
 *
 * @param x Character to check
 * @return bool true if whitespace, false otherwise
 */
bool isspace(const char x);

/**
 * @brief Check if a character is alphabetic.
 *
 * @param x Character to check
 * @return bool true if alphabetic, false otherwise
 */
bool isalpha(const char x);

/**
 * @brief Locate first occurrence of a character in a string.
 *
 * @param s String to search
 * @param c Character to find
 * @return char* Pointer to first occurrence or NULL if not found
 */
char* strchr(const char *s, int c);

/**
 * @brief Append string with length check.
 *
 * @param dst Destination buffer
 * @param src Source string
 * @param siz Size of destination buffer
 * @return uint32_t Length of string attempted to create (may exceed siz)
 */
uint32_t strlcat(char *dst, const char *src, uint32_t siz);

/**
 * @brief Copy string with length check.
 *
 * @param dst Destination buffer
 * @param src Source string
 * @param siz Size of destination buffer
 * @return uint32_t Length of source string
 */
uint32_t strlcpy(char *dst, const char *src, uint32_t siz);

/**
 * @brief Duplicate a string.
 *
 * @param string String to duplicate
 * @return char* Newly allocated copy of string
 */
char* strdup(const char* string);

/**
 * @brief Duplicate a string into the BASIC temporary string arena.
 *
 * Allocates storage for the given string inside the BASIC interpreter's
 * fixed-size string arena. The returned pointer remains valid until
 * the next call to gc(), which resets the arena.
 *
 * Unlike kmalloc(), this allocator never frees individual strings;
 * all strings are discarded together when gc() is invoked.
 *
 * @warning
 * - The returned string is immutable. Modifying it (e.g. appending
 *   with strlcat or writing past the terminator) will corrupt
 *   the string arena and cause undefined behaviour.
 * - Empty strings are interned: all calls with "" return the same
 *   canonical pointer.
 *
 * @param ctx    BASIC interpreter context containing arena state.
 * @param string NUL-terminated source string to duplicate.
 * @return const char* Pointer to the arena-allocated copy, or NULL on error.
 */
const char* gc_strdup(basic_ctx* ctx, const char* string);

/**
 * @brief Reset the BASIC temporary string arena.
 *
 * Reclaims all strings allocated with gc_strdup() by resetting the arena
 * pointer back to its initial position. This does not free memory back
 * to the system; it simply makes the entire arena available for reuse.
 *
 * Called automatically at the end of each line execution in the BASIC
 * interpreter.
 *
 * @param ctx BASIC interpreter context containing arena state.
 * @return int Always returns 1.
 */
int gc(basic_ctx* ctx);

/**
 * @brief Convert a hex string to integer.
 *
 * @param n1 Hex string (e.g. "1A3F")
 * @return uint64_t Converted value
 */
uint64_t hextoint(const char* n1);

/**
 * @brief Check if a character is a decimal digit.
 *
 * @param x Character to check
 * @return unsigned char Non-zero if digit, 0 otherwise
 */
unsigned char isdigit(const char x);

/**
 * @brief Check if a character is a hexadecimal digit.
 *
 * @param x Character to check
 * @return unsigned char Non-zero if hex digit, 0 otherwise
 */
unsigned char isxdigit(const char x);

/**
 * @brief Convert string to integer.
 *
 * @param s String containing integer
 * @return int Converted value
 */
int atoi(const char *s);

/**
 * @brief Convert string to 64-bit signed integer.
 *
 * @param s String containing integer
 * @param radix Base (e.g. 10 for decimal, 16 for hex)
 * @return int64_t Converted value
 */
int64_t atoll(const char *s, int radix);

/**
 * @brief Convert string to 64-bit unsigned integer.
 *
 * @param s String containing integer
 * @return uint64_t Converted value
 */
uint64_t atoull(const char *s);

/**
 * @brief Convert string to floating point.
 *
 * @param s String containing floating-point value
 * @param a Output pointer to double
 * @return bool true if successful, false if invalid
 */
bool atof(const char* s, double* a);

/**
 * @brief Absolute value of integer.
 *
 * @param a Integer
 * @return int Absolute value
 */
int abs(int a);

/**
 * @brief Absolute value of 64-bit integer.
 *
 * @param a 64-bit integer
 * @return int64_t Absolute value
 */
int64_t labs(int64_t a);

/**
 * @brief Reverse a string in-place.
 *
 * @param s String buffer
 * @return size_t Length of string
 */
size_t strrev(char* s);

/**
 * @brief Core atoi implementation.
 *
 * @param dst Destination for result
 * @param target String to convert
 * @param radix Base
 * @return int 0 on success, non-zero on error
 */
int do_atoi(int64_t* dst, char* target, unsigned radix);

/**
 * @brief Convert integer to string.
 *
 * @param target Integer value
 * @param buf Output buffer
 * @param radix Base
 * @return int 0 on success, non-zero on error
 */
int do_itoa(int64_t target, char* buf, unsigned radix);

/**
 * @brief Locate the last occurrence of a character in a string.
 *
 * @param s Pointer to the null-terminated string to search.
 * @param c Character to locate (passed as int, converted to unsigned char).
 * @return Pointer to the last occurrence of c in s, or NULL if not found.
 */
char* strrchr(const char* s, int c);

/**
 * @brief Case-insensitive string comparison.
 *
 * @param s1 First null-terminated string.
 * @param s2 Second null-terminated string.
 * @return <0 if s1 < s2, 0 if equal, >0 if s1 > s2 (case-insensitive).
 */
int strcasecmp(const char* s1, const char* s2);

/**
 * @brief Convert a string to lowercase in place.
 *
 * Iterates over the null-terminated string and replaces any
 * uppercase ASCII letters (A–Z) with their lowercase form.
 *
 * @param s Null-terminated string to modify.
 */
void strtolower(char *s);

/**
 * @brief Return visible length of a string ignoring ANSI escapes.
 *
 * Skips CSI sequences (ESC '[' ... final byte). Only counts characters
 * that would actually be displayed.
 *
 * @param s Null-terminated string.
 * @return Visible character count.
 */
size_t strlen_ansi(const char *s);

/**
 * @brief Locate a substring in a string.
 *
 * Scans the string @p haystack for the first occurrence of the
 * substring @p needle. The terminating null character is not
 * compared.
 *
 * If @p needle is empty, @p haystack is returned.
 * If @p needle does not occur in @p haystack, NULL is returned.
 *
 * @param haystack  Null-terminated string to search.
 * @param needle    Null-terminated substring to locate.
 *
 * @return Pointer to the beginning of the located substring,
 *         or NULL if no match is found.
 */
const char *strstr(const char *haystack, const char *needle);
