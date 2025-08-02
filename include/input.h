/**
 * @file kinput.h
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
 * @brief Read a character into the console input buffer.
 *
 * - Allocates a buffer on first use (`maxlen + 1` bytes).
 * - Appends typed characters to the buffer.
 * - Null-terminates the string after each call.
 *
 * @param maxlen Maximum buffer size (excluding null terminator).
 * @param cons Pointer to the console structure receiving input.
 * @return
 * - `0` if input is still ongoing,
 * - `1` if the line is complete (terminated with carriage return).
 */
size_t kinput(size_t maxlen, console* cons);

/**
 * @brief Free the internal console input buffer.
 *
 * Resets the console’s input state and releases memory allocated by `kinput()`.
 * Safe to call even if no buffer was allocated.
 *
 * @param cons Pointer to the console whose buffer should be freed.
 */
void kfreeinput(console* cons);

/**
 * @brief Retrieve the accumulated input string for a console.
 *
 * @param cons Pointer to the console.
 * @return Pointer to the null-terminated input string, or NULL if no buffer exists.
 *
 * @warning Do not free the returned pointer directly; use @ref kfreeinput().
 */
char* kgetinput(console* cons);
