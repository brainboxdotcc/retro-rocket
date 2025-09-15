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

#ifndef FLANTERM_FB_H
#define FLANTERM_FB_H 1

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../flanterm.h"

#ifdef FLANTERM_IN_FLANTERM

#include "fb_private.h"

#endif

/**
 * Initialise a framebuffer-backed flanterm context.
 *
 * Allocates and sets up all state required to render characters into a linear
 * framebuffer, using a supplied VGA-style font. The returned context implements
 * all standard flanterm operations, including cursor movement, colour setting,
 * scrolling, and character plotting.
 *
 * Font data is copied into @c ctx->font_bits and expanded into a boolean
 * lookup table @c ctx->font_bool. VGA rules for 9-dot wide characters are
 * observed: characters 0xC0–0xDF replicate the 9th column, others clear it.
 * Spacing is applied by widening @c ctx->font_width by @p font_spacing.
 *
 * The framebuffer is divided into cells of size @c glyph_width × @c glyph_height
 * (derived from font dimensions and scale factors). The number of rows and
 * columns is calculated automatically from the framebuffer size and margins.
 *
 * @param _malloc              Allocation function to use.
 * @param _free                Deallocation function to use.
 * @param framebuffer          Pointer to framebuffer base (ARGB).
 * @param width                Framebuffer width in pixels.
 * @param height               Framebuffer height in pixels.
 * @param pitch                Framebuffer pitch in bytes.
 * @param red_mask_size        Size of red channel mask in bits (must be 8).
 * @param red_mask_shift       Shift of red channel mask.
 * @param green_mask_size      Size of green channel mask in bits (must be 8).
 * @param green_mask_shift     Shift of green channel mask.
 * @param blue_mask_size       Size of blue channel mask in bits (must be 8).
 * @param blue_mask_shift      Shift of blue channel mask.
 * @param canvas               Optional backing canvas buffer, or NULL.
 * @param ansi_colours         Optional table of 8 ANSI colours, or NULL.
 * @param ansi_bright_colours  Optional table of 8 bright ANSI colours, or NULL.
 * @param default_bg           Optional default background colour, or NULL.
 * @param default_fg           Optional default foreground colour, or NULL.
 * @param default_bg_bright    Optional default bright background colour, or NULL.
 * @param default_fg_bright    Optional default bright foreground colour, or NULL.
 * @param font                 Pointer to font bitmap (font_height bytes per glyph).
 * @param font_width           Font width in pixels (typically 8).
 * @param font_height          Font height in pixels.
 * @param font_spacing         Extra spacing columns to add to font width.
 * @param font_scale_x         Horizontal font scaling factor.
 * @param font_scale_y         Vertical font scaling factor.
 * @param margin               Margin in pixels around the rendered grid.
 *
 * @return A pointer to an initialised @c flanterm_context on success, or NULL
 *         on failure. Callers are responsible for freeing it with deinit().
 */
struct flanterm_context *flanterm_fb_init(
	void *(*_malloc)(size_t size),
	void (*_free)(void *ptr, size_t size),
	uint32_t *framebuffer, size_t width, size_t height, size_t pitch,
	uint8_t red_mask_size, uint8_t red_mask_shift,
	uint8_t green_mask_size, uint8_t green_mask_shift,
	uint8_t blue_mask_size, uint8_t blue_mask_shift,
	uint32_t *canvas,
	uint32_t *ansi_colours, uint32_t *ansi_bright_colours,
	uint32_t *default_bg, uint32_t *default_fg,
	uint32_t *default_bg_bright, uint32_t *default_fg_bright,
	void *font, size_t font_width, size_t font_height, size_t font_spacing,
	size_t font_scale_x, size_t font_scale_y,
	size_t margin
);

/**
* Rebuild or update font glyph definitions in a framebuffer context.
*
* Behaviour depends on the value of @p glyph:
* - If @p glyph < 0: rebuild all glyphs from the existing @c ctx->font_bits
*   into @c ctx->font_bool.
* - If 0 <= @p glyph < FLANTERM_FB_FONT_GLYPHS:
*   - If @p bitmap is not NULL, copy @p bitmap (font_height bytes, one per row)
*     into @c ctx->font_bits for that glyph.
*   - Rebuild only the specified glyph in @c ctx->font_bool.
*
* VGA semantics are preserved: columns 0–7 are taken from the byte bitplane,
* column 8 is replicated for glyphs 0xC0–0xDF, and higher columns are cleared.
*
* This function does not alter the text grid, cursor state, colours, or
* rendering function pointers. Callers that require the new glyphs to be
* visible immediately should trigger a repaint (e.g. via full_refresh).
*
* @param _ctx   Generic flanterm context, must point to a fb context.
* @param glyph  Glyph index to update, or -1 to rebuild the entire atlas.
* @param bitmap Optional new bitmap data (font_height bytes) when updating a
*               single glyph. Ignored if @p glyph < 0.
*/
void flanterm_fb_update_font(struct flanterm_context *_ctx, int glyph, const uint8_t *bitmap);

/**
 * Render a NUL-terminated string directly into the framebuffer at a pixel position.
 *
 * This bypasses the text grid and update queue, drawing each glyph bitmap
 * directly to the back buffer. Each byte in the string indexes the font
 * glyphs 0x00–0xFF without UTF-8 decoding.
 *
 * @param _ctx           Flanterm context (must be an fb backend).
 * @param s              Pointer to a NUL-terminated 8-bit string to draw.
 * @param px             Pixel X coordinate of the top-left corner.
 * @param py             Pixel Y coordinate of the top-left corner.
 * @param fg_rgb         Foreground colour in 0x00RRGGBB format.
 * @param bg_rgb         Background colour in 0x00RRGGBB format.
 * @param transparent_bg When true, background pixels are left unchanged.
 *
 * The function clips drawing safely against framebuffer bounds. The pen
 * advances horizontally by the scaled glyph width per character, and moves
 * to the start of the next text row when encountering '\n'.
 */
void flanterm_fb_draw_text_px(struct flanterm_context *_ctx, const char *s, int32_t px, int32_t py, uint32_t fg_rgb, uint32_t bg_rgb, bool transparent_bg);

#ifdef __cplusplus
}
#endif

#endif
