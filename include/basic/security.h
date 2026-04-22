#pragma once

/**
 * @brief Represents a single restriction entry.
 *
 * Each restriction consists of a keyword or built-in function name and its
 * length. The string is expected to be a valid, null-terminated copy owned
 * by the allocator associated with the BASIC context.
 */
typedef struct restriction_t {
	size_t length;          /**< Length of the keyword string */
	const char* keyword;    /**< Keyword or built-in function name */
} restriction_t;

/**
 * @brief Iterator context used when copying restrictions to a child context.
 *
 * Used internally when scanning a parent context’s restriction set and
 * populating a child context with equivalent active restrictions.
 */
typedef struct restriction_iter_ctx_t {
	struct basic_ctx *parent; /**< Source context providing restrictions */
	struct basic_ctx *child;  /**< Destination context receiving restrictions */
} restriction_iter_ctx_t;

/**
 * @brief Adds a restriction to be inherited by child contexts.
 *
 * The provided keyword or built-in function name is stored in the calling
 * context’s child restriction set. Future child contexts will inherit this
 * restriction when initialised.
 *
 * @param ctx Current BASIC context
 * @param function_or_keyword Name of keyword or built-in function to restrict
 * @return true on success, false on error (e.g. allocation failure)
 */
bool basic_restrict_keyword_or_function_for_child(struct basic_ctx* ctx, const char* function_or_keyword);

/**
 * @brief Removes a restriction from the child inheritance set.
 *
 * If the specified keyword or built-in function is present in the child
 * restriction set, it is removed. Future child contexts will no longer
 * inherit this restriction.
 *
 * @param ctx Current BASIC context
 * @param function_or_keyword Name of keyword or built-in function to remove
 * @return true on success, false if not found or on error
 */
bool basic_derestrict_keyword_or_function_for_child(struct basic_ctx* ctx, const char* function_or_keyword);

/**
 * @brief Copies restrictions from a parent context into a child context.
 *
 * Initialises the child context’s active restriction set by duplicating
 * both the parent’s active restrictions and the parent’s child restriction
 * set. This ensures restrictions propagate down the interpreter chain.
 *
 * @param parent Source BASIC context
 * @param child Destination BASIC context
 * @return true on success, false on allocation failure
 */
bool basic_pass_restrictions_to_child(struct basic_ctx* parent, struct basic_ctx* child);

/**
 * @brief Checks if a keyword or built-in function is restricted.
 *
 * Performs a lookup in the active restriction set of the provided context.
 *
 * @param ctx BASIC context to check
 * @param function_or_keyword Name to test
 * @return true if restricted, false otherwise
 */
bool is_restricted(struct basic_ctx* ctx, const char* function_or_keyword);

/**
 * @brief Checks if a keyword or built-in function is restricted using a known length.
 *
 * Variant of is_restricted() that avoids recomputing string length.
 *
 * @param ctx BASIC context to check
 * @param function_or_keyword Name to test
 * @param l Length of the name
 * @return true if restricted, false otherwise
 */
bool is_restricted_len(struct basic_ctx* ctx, const char* function_or_keyword, size_t l);

/**
 * @brief Implements the RESTRICT statement.
 *
 * Parses a comma-separated list of keywords and built-in function names
 * from the current line and adds them to the child restriction set.
 *
 * @param ctx Current BASIC context
 */
void restrict_statement(struct basic_ctx *ctx);

/**
 * @brief Implements the DERESTRICT statement.
 *
 * Parses a comma-separated list of keywords and built-in function names
 * from the current line and removes them from the child restriction set.
 *
 * @param ctx Current BASIC context
 */
void derestrict_statement(struct basic_ctx *ctx);
