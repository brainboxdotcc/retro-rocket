#pragma once
#include <kernel.h>

/**
 * @brief Initialise the global dprintf line buffer.
 *
 * Allocates a fixed-size circular buffer with @p cap_bytes capacity. If
 * @p cap_bytes is zero, a default of 1 MiB is used. Existing content is
 * discarded. If a prior buffer exists, it is freed.
 *
 * @param cap_bytes Capacity in bytes (0 for 1 MiB).
 * @return true on success; false if allocation fails.
 *
 * @pre Caller must hold the debug-log lock (or be in a safe context).
 */
bool dprintf_buffer_init(size_t cap_bytes);

/**
 * @brief Append a single line to the dprintf buffer (whole-line eviction).
 *
 * Appends @p line (trailing newline optional). If there is not enough free
 * space, removes whole lines from the start (oldest data) until the new line
 * fits. If the input is larger than the buffer capacity, only the final
 * (cap-1) bytes are kept and a newline is forced, ensuring forward progress.
 *
 * @param line Pointer to bytes to append (may be non-terminated).
 * @param len  Number of bytes at @p line (newline optional).
 *
 * @pre Caller must hold the debug-log lock (or be in a safe context).
 */
void dprintf_buffer_append_line(const char *line, size_t len);

/**
 * @brief Get a contiguous, null-terminated snapshot of the debug log buffer.
 *
 * The returned string contains the entire contents of the buffer from the
 * oldest byte to the newest, in order. It is always null-terminated. If the
 * buffer is empty, the returned string will be an empty string.
 *
 * @return Pointer to a kmalloc()'d string containing the buffer contents.
 *         Caller is responsible for kfree()'ing it. Returns NULL on allocation
 *         failure or if the buffer has not been initialised.
 *
 * @pre Caller must hold the debug-log lock (or be in a safe context).
 */
char *dprintf_buffer_snapshot(void);

size_t dprintf_size();
