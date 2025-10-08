#pragma once
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Result codes returned by the regex API.
 *
 * Positive values indicate partial or non-match outcomes.
 * Negative values indicate internal or fatal errors.
 */
enum re_res {
	RE_OK = 0,        /**< Successful match or compile. */
	RE_NOMATCH = 1,   /**< Pattern compiled but no match found. */
	RE_AGAIN = -10001,/**< Cooperative execution not yet complete. */
	RE_EINVAL = -10002,/**< Invalid argument or malformed input. */
	RE_ECODE = -10003, /**< Internal regex engine error. */
	RE_EMEM = -10004   /**< Memory allocation failure. */
};

/** @brief Opaque compiled regular expression program. */
struct regex_prog;

/**
 * @brief Result of a single match attempt.
 *
 * Contains basic match state and offsets into the haystack buffer.
 */
struct regex_match {
	bool matched;        /**< True if pattern matched. */
	size_t start_offset; /**< Start offset in bytes. */
	size_t end_offset;   /**< End offset in bytes (one past). */
};

/**
 * @brief Compile a counted, byte-only pattern.
 *
 * Wraps `regcomp()` using ASCII-only semantics.
 *
 * @param allocator Memory allocator to use (buddy allocator).
 * @param out Receives compiled program handle on success.
 * @param pat Pointer to byte pattern (not necessarily NUL-terminated).
 * @param pat_len Length of pattern in bytes.
 * @return `RE_OK` on success, or a negative `re_res` code on failure.
 * @note If @p out is non-NULL on error, the caller is expected to free it with regex_free()
 */
int regex_compile(buddy_allocator_t *allocator, struct regex_prog **out, const uint8_t *pat, size_t pat_len);

/**
 * @brief Free a compiled regular expression.
 *
 * Destroys the internal program and releases associated memory.
 *
 * @param p Program handle returned by @ref regex_compile.
 */
void regex_free(struct regex_prog *p);

/**
 * @brief Execute one cooperative slice of a regular expression.
 *
 * May return `RE_AGAIN` if the scan should resume from the returned
 * `*next_off` offset.  Call repeatedly until a terminal state is reached.
 *
 * @param p Compiled program to execute.
 * @param hay Pointer to input buffer.
 * @param hay_len Length of input buffer in bytes.
 * @param start_off Offset to begin matching from.
 * @param m Output match structure (may be NULL if only status is required).
 * @param next_off On return, updated resume offset.
 * @return `RE_OK`, `RE_NOMATCH`, or `RE_AGAIN`.
 */
int regex_exec_slice(const struct regex_prog *p, const uint8_t *hay, size_t hay_len, size_t start_off, struct regex_match *m, size_t *next_off);

/**
 * @brief Run an internal self-test for the regex subsystem.
 *
 * Emits diagnostic output via `dprintf()` and validates known-good cases.
 * Intended for developer regression testing only.
 */
void regex_selftest(void);

/**
 * @brief Return a human-readable error for the last failure.
 *
 * Retrieves a descriptive error message for the last compile or
 * execution failure associated with the given program.
 *
 * @param p Program handle.
 * @return Pointer to static string, or empty string if none available.
 */
const char *regex_last_error(const struct regex_prog *p);
