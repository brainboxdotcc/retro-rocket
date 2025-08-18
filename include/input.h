/**
 * @file input.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @brief Console line input API for Retro Rocket.
 *
 * This interface provides buffered, line-based input for consoles. It handles
 * character input, backspace editing, line completion detection, and safe
 * buffer allocation. The functions in this header are backed by the
 * implementation in @ref input.c.
 *
 * ## Usage Lifecycle
 * 1. Call `kinput(maxlen, cons)` repeatedly until it returns `1` (line complete).
 * 2. Retrieve the full line with `kgetinput(cons)`.
 * 3. Process the string as needed.
 * 4. Call `kfreeinput(cons)` when finished to release the internal buffer.
 *
 * The buffer is lazily allocated by `kinput()` when the first character arrives,
 * so you must not call `kgetinput()` until after at least one successful call to
 * `kinput()`.
 *
 * ## Behaviour
 * - Input is null-terminated automatically.
 * - Carriage return (`'\r'`) signals end-of-line and is echoed as newline (`'\n'`).
 * - Backspace (`8`) removes the previous character (with console cursor handling).
 * - Arrow keys (↑ ↓ ← →) are recognised but ignored.
 * - If the buffer limit is reached, no more characters are added and a beep is
 *   emitted as feedback.
 *
 * @note This input API is retained primarily to support the BASIC INPUT
 *       statement and similar minimal line-reading requirements. Modern
 *       interactive shells and tools use the ANSI console library, which
 *       provides line editing, history, and richer input handling.
 *
 *       `kinput()` is intentionally simple and blocking; it does not support
 *       advanced editing or multiple input sources.
 */

#pragma once

/**
 * @brief Poll for a single character of input from the console.
 *
 * This function reads a single character from the console associated
 * with the specified process, storing it in the input buffer.
 *
 * When the user completes a line (e.g. by pressing Enter), the function
 * returns 1 to indicate the input line is ready for retrieval.
 * Until then, it returns 0, allowing the caller to yield or retry later.
 *
 * @note This function is typically called repeatedly inside a loop or
 * idle callback, until a full line of input is received.
 *
 * @param maxlen Maximum length of the input line.
 * @param cons Console to read input from.
 * @return 1 if a full line is available, 0 otherwise.
 */
size_t kinput(size_t maxlen);

/**
 * @brief Free the internal input buffer associated with a console.
 *
 * This function releases the memory used to store a pending line of
 * input for the specified console. It should be called after input
 * has been processed, to prevent memory leaks and prepare the console
 * for the next `kinput()` cycle.
 *
 * @param cons Console whose input buffer should be freed.
 */
void kfreeinput();

/**
 * @brief Retrieve the accumulated input string for a console.
 *
 * @param cons Pointer to the console.
 * @return Pointer to the null-terminated input string, or NULL if no buffer exists.
 *
 * @warning Do not free the returned pointer directly; use @ref kfreeinput().
 */
char* kgetinput();
