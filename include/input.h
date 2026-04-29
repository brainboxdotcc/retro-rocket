/**
 * @file input.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @brief Simple line input API backed by a per-context buffer.
 *
 * Provides minimal, line-based input suitable for BASIC `INPUT` and
 * similar use cases. Input is accumulated into a caller-owned context
 * and completed when a carriage return is received.
 *
 * ## Usage
 * 1. Initialise with `kinitinput(ctx)`.
 * 2. Call `kinput(ctx)` until it returns true (line complete).
 * 3. Read the line with `kgetinput(ctx)`.
 * 4. Call `kfreeinput(ctx)` when finished.
 *
 * ## Behaviour
 * - Input is null-terminated.
 * - `'\r'` completes the line and is echoed as `'\n'`.
 * - Backspace (`8`) deletes the previous character with cursor update.
 * - Arrow keys are ignored.
 * - Buffer grows dynamically as needed.
 *
 * @note This API is polled and intended for cooperative scheduling.
 *       It performs no advanced line editing.
 */

#pragma once

struct basic_ctx;

/**
 * @brief Per-input context state.
 *
 * Stores the buffer and cursor state for a single line input operation.
 */
typedef struct buffered_input_context_t {
	bool stopped;		/**< Line completion flag */
	uint8_t last;		/**< Last character read */
	char* internalbuffer;	/**< Base pointer to allocated buffer */
	char* buffer;		/**< Current write pointer */
	size_t bufcnt;		/**< Number of characters in buffer */
	size_t buflen;		/**< Allocated buffer size */
} buffered_input_context_t;

/**
 * @brief Initialise an input context.
 *
 * Clears all state. Must be called before first use.
 *
 * @param ctx Input context
 */
void kinitinput(struct buffered_input_context_t* ctx);

/**
 * @brief Poll for input and update the buffer.
 *
 * Reads a single character and updates the context buffer.
 *
 * @param basic BASIC context
 * @param ctx Input context
 * @return true if a full line has been entered, false otherwise
 */
bool kinput(struct basic_ctx* basic, struct buffered_input_context_t* ctx);

/**
 * @brief Free any allocated input buffer and reset the context.
 *
 * @param basic BASIC context
 * @param ctx Input context
 */
void kfreeinput(struct basic_ctx* basic, struct buffered_input_context_t* ctx);

/**
 * @brief Get the current input buffer.
 *
 * @param ctx Input context
 * @return Pointer to null-terminated string, or NULL if not initialised
 *
 * @warning Do not free the returned pointer directly
 */
const char* kgetinput(struct buffered_input_context_t* ctx);