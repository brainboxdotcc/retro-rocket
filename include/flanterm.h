/* Copyright (C) 2022-2025 mintsuki and contributors.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Retro Rocket fork notice
 *
 * This file is part of the Retro Rocket operating system and contains a
 * maintained fork of the flanterm terminal implementation originally
 * written by mintsuki.
 *
 * Modified portions (C) Craig Edwards, 2025-2026
 *
 * Purpose of fork:
 *     Integration with the Retro Rocket graphics and terminal subsystem.
 *     This fork introduces additional functionality required by the Retro
 *     Rocket console environment, including:
 *
 *     - native cursor and colour control interfaces
 *     - scroll callbacks for synchronising text and graphics
 *     - dirty region tracking for framebuffer updates
 *     - glyph redefinition handling
 *
 * Because of these extensions, this implementation is not drop-in compatible
 * with upstream flanterm and should not be replaced with an upstream version
 * without adapting the Retro Rocket specific interfaces.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Callback type for DEC private control sequences.
 *
 * This is triggered when the terminal receives a DEC private sequence
 * requiring backend handling.
 */
#define FLANTERM_CB_DEC 10

/**
 * Callback type for terminal bell requests.
 *
 * This is triggered when the BEL control character is received.
 */
#define FLANTERM_CB_BELL 20

/**
 * Callback type for terminal identification requests.
 *
 * This is triggered when a private device identification sequence
 * is processed.
 */
#define FLANTERM_CB_PRIVATE_ID 30

/**
 * Callback type for terminal status report requests.
 *
 * This is triggered when the terminal receives a device status query.
 */
#define FLANTERM_CB_STATUS_REPORT 40

/**
 * Callback type for cursor position reports.
 *
 * This is triggered when the terminal processes a request to report
 * the current cursor position.
 */
#define FLANTERM_CB_POS_REPORT 50

/**
 * Callback type for keyboard LED state updates.
 *
 * This is triggered when a sequence attempts to update keyboard LED
 * indicators such as Caps Lock, Num Lock, or Scroll Lock.
 */
#define FLANTERM_CB_KBD_LEDS 60

/**
 * Callback type for terminal mode changes.
 *
 * This is triggered when the terminal receives a sequence that modifies
 * terminal operating modes.
 */
#define FLANTERM_CB_MODE 70

/**
 * Callback type for Linux-specific control sequences.
 *
 * This is triggered when Linux console private escape sequences are
 * processed.
 */
#define FLANTERM_CB_LINUX 80

/**
 * Callback type for terminal scroll events.
 *
 * This is a Retro Rocket fork extension triggered when the terminal
 * scrolls its text region.
 */
#define FLANTERM_CB_SCROLL 90

#define FLANTERM_OOB_OUTPUT_OCRNL (1 << 0)
#define FLANTERM_OOB_OUTPUT_OFDEL (1 << 1)
#define FLANTERM_OOB_OUTPUT_OFILL (1 << 2)
#define FLANTERM_OOB_OUTPUT_OLCUC (1 << 3)
#define FLANTERM_OOB_OUTPUT_ONLCR (1 << 4)
#define FLANTERM_OOB_OUTPUT_ONLRET (1 << 5)
#define FLANTERM_OOB_OUTPUT_ONOCR (1 << 6)
#define FLANTERM_OOB_OUTPUT_OPOST (1 << 7)

#ifdef FLANTERM_IN_FLANTERM

#include "flanterm_private.h"

#else

struct flanterm_context;

#endif

/**
 * Write a buffer of terminal data to the flanterm context.
 *
 * The buffer is parsed as terminal input and may contain printable text,
 * control characters, or escape sequences.
 *
 * @param ctx Flanterm context to write to.
 * @param buf Buffer containing terminal data.
 * @param count Number of bytes to write from the buffer.
 */
void flanterm_write(struct flanterm_context *ctx, const char *buf, size_t count);

/**
 * Flush any pending terminal output to the backing surface.
 *
 * This commits any buffered rendering work for the current context.
 *
 * @param ctx Flanterm context to flush.
 */
void flanterm_flush(struct flanterm_context *ctx);

/**
 * Force a full redraw of the terminal surface.
 *
 * This refreshes the entire terminal output rather than only changed regions.
 *
 * @param ctx Flanterm context to refresh.
 */
void flanterm_full_refresh(struct flanterm_context *ctx);

/**
 * Deinitialise a flanterm context and release any resources it owns.
 *
 * The supplied free callback is used to release allocations associated with
 * the context.
 *
 * @param ctx Flanterm context to destroy.
 * @param _free Allocator-compatible free function.
 */
void flanterm_deinit(struct flanterm_context *ctx, void (*_free)(void *ptr, size_t size));

/**
 * Get the terminal dimensions in character cells.
 *
 * @param ctx Flanterm context to query.
 * @param cols Receives the number of text columns.
 * @param rows Receives the number of text rows.
 */
void flanterm_get_dimensions(struct flanterm_context *ctx, size_t *cols, size_t *rows);

/**
 * Enable or disable automatic flushing after writes.
 *
 * When enabled, writes are flushed automatically. When disabled, callers
 * must flush explicitly.
 *
 * @param ctx Flanterm context to update.
 * @param state New autoflush state.
 */
void flanterm_set_autoflush(struct flanterm_context *ctx, bool state);

/**
 * Set the callback used for terminal events.
 *
 * The callback receives terminal notifications such as bell requests,
 * reports, and any fork-specific callback extensions.
 *
 * @param ctx Flanterm context to update.
 * @param callback Callback function, or NULL to clear it.
 */
void flanterm_set_callback(struct flanterm_context *ctx, void (*callback)(struct flanterm_context *, uint64_t, uint64_t, uint64_t, uint64_t));

/**
 * Get the current out-of-band output mode flags.
 *
 * @param ctx Flanterm context to query.
 *
 * @return Current out-of-band output flags.
 */
uint64_t flanterm_get_oob_output(struct flanterm_context *ctx);

/**
 * Set the out-of-band output mode flags.
 *
 * @param ctx Flanterm context to update.
 * @param oob_output New out-of-band output flags.
 */
void flanterm_set_oob_output(struct flanterm_context *ctx, uint64_t oob_output);

/**
 * Get the minimum Y position touched by the most recent rendering work.
 *
 * This is a Retro Rocket fork extension used for dirty-region tracking.
 *
 * @return Minimum modified Y coordinate.
 */
int64_t flanterm_ex_get_bounding_min_y(void);

/**
 * Get the maximum Y position touched by the most recent rendering work.
 *
 * This is a Retro Rocket fork extension used for dirty-region tracking.
 *
 * @return Maximum modified Y coordinate.
 */
int64_t flanterm_ex_get_bounding_max_y(void);

/**
 * Mark a single-byte character as having a redefined glyph.
 *
 * Redefined bytes are treated as literal single-byte glyphs rather than
 * as possible UTF-8 lead or continuation bytes.
 *
 * @param c Character byte whose glyph has been redefined.
 */
void ft_mark_redefined(uint8_t c);

/**
 * Get the current cursor position.
 *
 * The returned coordinates are zero-based.
 *
 * @param ctx Flanterm context to query.
 * @param x Receives the cursor X position.
 * @param y Receives the cursor Y position.
 */
void flanterm_get_cursor_pos(struct flanterm_context *ctx, size_t *x, size_t *y);

/**
 * Set the current cursor position.
 *
 * Coordinates are zero-based.
 *
 * @param ctx Flanterm context to update.
 * @param x New cursor X position.
 * @param y New cursor Y position.
 */
void flanterm_set_cursor_pos(struct flanterm_context *ctx, size_t x, size_t y);

/**
 * Set the current text foreground colour.
 *
 * The colour index uses the terminal's base 0-7 colour range. Brightness is
 * controlled separately by the bright flag.
 *
 * @param ctx Flanterm context to update.
 * @param colour Base foreground colour index.
 * @param bright True for bright/intense output, false for normal intensity.
 */
void flanterm_set_text_fg(struct flanterm_context *ctx, size_t colour, bool bright);

/**
 * Set the current text background colour.
 *
 * The colour index uses the terminal's base 0-7 colour range. Brightness is
 * controlled separately by the bright flag.
 *
 * @param ctx Flanterm context to update.
 * @param colour Base background colour index.
 * @param bright True for bright/intense output, false for normal intensity.
 */
void flanterm_set_text_bg(struct flanterm_context *ctx, size_t colour, bool bright);

/**
 * Reset the foreground colour to the current default.
 *
 * This preserves the terminal's internal attribute model and applies the
 * appropriate default foreground for the current state.
 *
 * @param ctx Flanterm context to update.
 */
void flanterm_reset_text_fg(struct flanterm_context *ctx);

/**
 * Reset the background colour to the current default.
 *
 * This preserves the terminal's internal attribute model and applies the
 * appropriate default background for the current state.
 *
 * @param ctx Flanterm context to update.
 */
void flanterm_reset_text_bg(struct flanterm_context *ctx);

/**
 * Clear the terminal contents.
 *
 * When move is true, the cursor is also repositioned according to the
 * backend clear implementation.
 *
 * @param ctx Flanterm context to clear.
 * @param move True to reposition the cursor as part of the clear.
 */
void flanterm_clear(struct flanterm_context *ctx, bool move);

