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

#ifdef __cplusplus
#error "Please do not compile Flanterm as C++ code! Flanterm should be compiled as C99 or newer."
#endif

#ifndef __STDC_VERSION__
#error "Flanterm must be compiled as C99 or newer."
#endif

#if defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define ALWAYS_INLINE inline
#endif

#include "kernel.h"

#define FLANTERM_IN_FLANTERM

#include "flanterm.h"
#include "flanterm/fb.h"

static int64_t ft_min_y = -1;
static int64_t ft_max_y = -1;

static ALWAYS_INLINE uint32_t convert_colour(struct flanterm_context *_ctx, uint32_t colour) {
	struct flanterm_fb_context *ctx = (void *) _ctx;
	uint32_t r = (colour >> 16) & 0xff;
	uint32_t g = (colour >> 8) & 0xff;
	uint32_t b = colour & 0xff;
	return (r << ctx->red_mask_shift) | (g << ctx->green_mask_shift) | (b << ctx->blue_mask_shift);
}

static void flanterm_fb_save_state(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;
	ctx->saved_state_text_fg = ctx->text_fg;
	ctx->saved_state_text_bg = ctx->text_bg;
	ctx->saved_state_cursor_x = ctx->cursor_x;
	ctx->saved_state_cursor_y = ctx->cursor_y;
}

static void flanterm_fb_restore_state(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;
	ctx->text_fg = ctx->saved_state_text_fg;
	ctx->text_bg = ctx->saved_state_text_bg;
	ctx->cursor_x = ctx->saved_state_cursor_x;
	ctx->cursor_y = ctx->saved_state_cursor_y;
}

static void flanterm_fb_swap_palette(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;
	uint32_t tmp = ctx->text_bg;
	ctx->text_bg = ctx->text_fg;
	ctx->text_fg = tmp;
}

static void plot_char_scaled_canvas(struct flanterm_context *_ctx, struct flanterm_fb_char *c, size_t x, size_t y) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	if (x >= _ctx->cols || y >= _ctx->rows) {
		return;
	}

	x = ctx->offset_x + x * ctx->glyph_width;
	y = ctx->offset_y + y * ctx->glyph_height;

	bool *glyph = &ctx->font_bool[c->c * ctx->font_height * ctx->font_width];
	// naming: fx,fy for font coordinates, gx,gy for glyph coordinates
	for (size_t gy = 0; gy < ctx->glyph_height; gy++) {
		uint8_t fy = gy / ctx->font_scale_y;
		volatile uint32_t *fb_line = ctx->framebuffer + x + (y + gy) * (ctx->pitch / 4);
		uint32_t *canvas_line = ctx->canvas + x + (y + gy) * ctx->width;
		bool *glyph_pointer = glyph + (fy * ctx->font_width);
		for (size_t fx = 0; fx < ctx->font_width; fx++) {
			for (size_t i = 0; i < ctx->font_scale_x; i++) {
				size_t gx = ctx->font_scale_x * fx + i;
				uint32_t bg = c->bg == 0xffffffff ? canvas_line[gx] : c->bg;
				uint32_t fg = c->fg == 0xffffffff ? canvas_line[gx] : c->fg;
				fb_line[gx] = *glyph_pointer ? fg : bg;
			}
			glyph_pointer++;
		}
	}
}

static void plot_char_scaled_uncanvas(struct flanterm_context *_ctx, struct flanterm_fb_char *c, size_t x, size_t y) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	if (x >= _ctx->cols || y >= _ctx->rows) {
		return;
	}

	uint32_t default_bg = ctx->default_bg;

	uint32_t bg = c->bg == 0xffffffff ? default_bg : c->bg;
	uint32_t fg = c->fg == 0xffffffff ? default_bg : c->fg;

	x = ctx->offset_x + x * ctx->glyph_width;
	y = ctx->offset_y + y * ctx->glyph_height;

	bool *glyph = &ctx->font_bool[c->c * ctx->font_height * ctx->font_width];
	// naming: fx,fy for font coordinates, gx,gy for glyph coordinates
	for (size_t gy = 0; gy < ctx->glyph_height; gy++) {
		uint8_t fy = gy / ctx->font_scale_y;
		volatile uint32_t *fb_line = ctx->framebuffer + x + (y + gy) * (ctx->pitch / 4);
		bool *glyph_pointer = glyph + (fy * ctx->font_width);
		for (size_t fx = 0; fx < ctx->font_width; fx++) {
			for (size_t i = 0; i < ctx->font_scale_x; i++) {
				size_t gx = ctx->font_scale_x * fx + i;
				fb_line[gx] = *glyph_pointer ? fg : bg;
			}
			glyph_pointer++;
		}
	}
}

static void plot_char_unscaled_canvas(struct flanterm_context *_ctx, struct flanterm_fb_char *c, size_t x, size_t y) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	if (x >= _ctx->cols || y >= _ctx->rows) {
		return;
	}

	x = ctx->offset_x + x * ctx->glyph_width;
	y = ctx->offset_y + y * ctx->glyph_height;

	bool *glyph = &ctx->font_bool[c->c * ctx->font_height * ctx->font_width];
	// naming: fx,fy for font coordinates, gx,gy for glyph coordinates
	for (size_t gy = 0; gy < ctx->glyph_height; gy++) {
		volatile uint32_t *fb_line = ctx->framebuffer + x + (y + gy) * (ctx->pitch / 4);
		uint32_t *canvas_line = ctx->canvas + x + (y + gy) * ctx->width;
		bool *glyph_pointer = glyph + (gy * ctx->font_width);
		for (size_t fx = 0; fx < ctx->font_width; fx++) {
			uint32_t bg = c->bg == 0xffffffff ? canvas_line[fx] : c->bg;
			uint32_t fg = c->fg == 0xffffffff ? canvas_line[fx] : c->fg;
			fb_line[fx] = *(glyph_pointer++) ? fg : bg;
		}
	}
}

int64_t flanterm_ex_get_bounding_min_y() {
	return ft_min_y;
}

int64_t flanterm_ex_get_bounding_max_y() {
	return ft_max_y;
}

static void plot_char_unscaled_uncanvas(struct flanterm_context *_ctx, struct flanterm_fb_char *c, size_t x, size_t y) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	if (x >= _ctx->cols || y >= _ctx->rows) {
		return;
	}

	uint32_t default_bg = ctx->default_bg;

	uint32_t bg = c->bg == 0xffffffff ? default_bg : c->bg;
	uint32_t fg = c->fg == 0xffffffff ? default_bg : c->fg;

	x = ctx->offset_x + x * ctx->glyph_width;
	y = ctx->offset_y + y * ctx->glyph_height;

	bool *glyph = &ctx->font_bool[c->c * ctx->font_height * ctx->font_width];
	// naming: fx,fy for font coordinates, gx,gy for glyph coordinates
	if (ft_min_y == -1 || ft_min_y > (int64_t) y) {
		ft_min_y = (int64_t) y;
	}
	if (ft_max_y == -1 || ft_max_y < (int64_t) (y + ctx->glyph_height)) {
		ft_max_y = (int64_t) (y + ctx->glyph_height);
	}
	for (size_t gy = 0; gy < ctx->glyph_height; gy++) {
		volatile uint32_t *fb_line = ctx->framebuffer + x + (y + gy) * (ctx->pitch / 4);
		bool *glyph_pointer = glyph + (gy * ctx->font_width);
		for (size_t fx = 0; fx < ctx->font_width; fx++) {
			fb_line[fx] = *(glyph_pointer++) ? fg : bg;
		}
	}
}

static inline bool compare_char(struct flanterm_fb_char *a, struct flanterm_fb_char *b) {
	return !(a->c != b->c || a->bg != b->bg || a->fg != b->fg);
}

static void push_to_queue(struct flanterm_context *_ctx, struct flanterm_fb_char *c, size_t x, size_t y) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	if (x >= _ctx->cols || y >= _ctx->rows) {
		return;
	}

	size_t i = y * _ctx->cols + x;

	struct flanterm_fb_queue_item *q = ctx->map[i];

	if (q == NULL) {
		if (compare_char(&ctx->grid[i], c)) {
			return;
		}
		q = &ctx->queue[ctx->queue_i++];
		q->x = x;
		q->y = y;
		ctx->map[i] = q;
	}

	q->c = *c;
}

static void flanterm_fb_revscroll(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	for (size_t i = (_ctx->scroll_bottom_margin - 1) * _ctx->cols - 1;
	     i >= _ctx->scroll_top_margin * _ctx->cols; i--) {
		if (i == (size_t) -1) {
			break;
		}
		struct flanterm_fb_char *c;
		struct flanterm_fb_queue_item *q = ctx->map[i];
		if (q != NULL) {
			c = &q->c;
		} else {
			c = &ctx->grid[i];
		}
		push_to_queue(_ctx, c, (i + _ctx->cols) % _ctx->cols, (i + _ctx->cols) / _ctx->cols);
	}

	// Clear the first line of the screen.
	struct flanterm_fb_char empty;
	empty.c = ' ';
	empty.fg = ctx->text_fg;
	empty.bg = ctx->text_bg;
	for (size_t i = 0; i < _ctx->cols; i++) {
		push_to_queue(_ctx, &empty, i, _ctx->scroll_top_margin);
	}
}

static void flanterm_fb_scroll(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	for (size_t i = (_ctx->scroll_top_margin + 1) * _ctx->cols; i < _ctx->scroll_bottom_margin * _ctx->cols; i++) {
		struct flanterm_fb_char *c;
		struct flanterm_fb_queue_item *q = ctx->map[i];
		if (q != NULL) {
			c = &q->c;
		} else {
			c = &ctx->grid[i];
		}
		push_to_queue(_ctx, c, (i - _ctx->cols) % _ctx->cols, (i - _ctx->cols) / _ctx->cols);
	}

	// Clear the last line of the screen.
	struct flanterm_fb_char empty;
	empty.c = ' ';
	empty.fg = ctx->text_fg;
	empty.bg = ctx->text_bg;
	for (size_t i = 0; i < _ctx->cols; i++) {
		push_to_queue(_ctx, &empty, i, _ctx->scroll_bottom_margin - 1);
	}

	if (_ctx->callback != NULL) {
		_ctx->callback(_ctx, FLANTERM_CB_SCROLL, 0, 0, 0);
	}
}

static void flanterm_fb_clear(struct flanterm_context *_ctx, bool move) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	struct flanterm_fb_char empty;
	empty.c = ' ';
	empty.fg = ctx->text_fg;
	empty.bg = ctx->text_bg;
	for (size_t i = 0; i < _ctx->rows * _ctx->cols; i++) {
		push_to_queue(_ctx, &empty, i % _ctx->cols, i / _ctx->cols);
	}

	if (move) {
		ctx->cursor_x = 0;
		ctx->cursor_y = 0;
	}
}

static void flanterm_fb_set_cursor_pos(struct flanterm_context *_ctx, size_t x, size_t y) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	if (x >= _ctx->cols) {
		if ((int) x < 0) {
			x = 0;
		} else {
			x = _ctx->cols - 1;
		}
	}
	if (y >= _ctx->rows) {
		if ((int) y < 0) {
			y = 0;
		} else {
			y = _ctx->rows - 1;
		}
	}
	ctx->cursor_x = x;
	ctx->cursor_y = y;
}

static void flanterm_fb_get_cursor_pos(struct flanterm_context *_ctx, size_t *x, size_t *y) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	*x = ctx->cursor_x >= _ctx->cols ? _ctx->cols - 1 : ctx->cursor_x;
	*y = ctx->cursor_y >= _ctx->rows ? _ctx->rows - 1 : ctx->cursor_y;
}

static void flanterm_fb_move_character(struct flanterm_context *_ctx, size_t new_x, size_t new_y, size_t old_x, size_t old_y) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	if (old_x >= _ctx->cols || old_y >= _ctx->rows
	    || new_x >= _ctx->cols || new_y >= _ctx->rows) {
		return;
	}

	size_t i = old_x + old_y * _ctx->cols;

	struct flanterm_fb_char *c;
	struct flanterm_fb_queue_item *q = ctx->map[i];
	if (q != NULL) {
		c = &q->c;
	} else {
		c = &ctx->grid[i];
	}

	push_to_queue(_ctx, c, new_x, new_y);
}

static void flanterm_fb_set_text_fg(struct flanterm_context *_ctx, size_t fg) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ctx->text_fg = ctx->ansi_colours[fg];
}

static void flanterm_fb_set_text_bg(struct flanterm_context *_ctx, size_t bg) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ctx->text_bg = ctx->ansi_colours[bg];
}

static void flanterm_fb_set_text_fg_bright(struct flanterm_context *_ctx, size_t fg) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ctx->text_fg = ctx->ansi_bright_colours[fg];
}

static void flanterm_fb_set_text_bg_bright(struct flanterm_context *_ctx, size_t bg) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ctx->text_bg = ctx->ansi_bright_colours[bg];
}

static void flanterm_fb_set_text_fg_rgb(struct flanterm_context *_ctx, uint32_t fg) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ctx->text_fg = convert_colour(_ctx, fg);
}

static void flanterm_fb_set_text_bg_rgb(struct flanterm_context *_ctx, uint32_t bg) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ctx->text_bg = convert_colour(_ctx, bg);
}

static void flanterm_fb_set_text_fg_default(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ctx->text_fg = ctx->default_fg;
}

static void flanterm_fb_set_text_bg_default(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ctx->text_bg = 0xffffffff;
}

static void flanterm_fb_set_text_fg_default_bright(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ctx->text_fg = ctx->default_fg_bright;
}

static void flanterm_fb_set_text_bg_default_bright(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ctx->text_bg = ctx->default_bg_bright;
}

static void draw_cursor(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	if (ctx->cursor_x >= _ctx->cols || ctx->cursor_y >= _ctx->rows) {
		return;
	}

	size_t i = ctx->cursor_x + ctx->cursor_y * _ctx->cols;

	struct flanterm_fb_char c;
	struct flanterm_fb_queue_item *q = ctx->map[i];
	if (q != NULL) {
		c = q->c;
	} else {
		c = ctx->grid[i];
	}
	uint32_t tmp = c.fg;
	c.fg = c.bg;
	c.bg = tmp;
	ctx->plot_char(_ctx, &c, ctx->cursor_x, ctx->cursor_y);
	if (q != NULL) {
		ctx->grid[i] = q->c;
		ctx->map[i] = NULL;
	}
}

static void flanterm_fb_double_buffer_flush(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	ft_min_y = -1;
	ft_max_y = -1;

	if (_ctx->cursor_enabled) {
		draw_cursor(_ctx);
	}

	for (size_t i = 0; i < ctx->queue_i; i++) {
		struct flanterm_fb_queue_item *q = &ctx->queue[i];
		size_t offset = q->y * _ctx->cols + q->x;
		if (ctx->map[offset] == NULL) {
			continue;
		}
		ctx->plot_char(_ctx, &q->c, q->x, q->y);
		ctx->grid[offset] = q->c;
		ctx->map[offset] = NULL;
	}

	if ((ctx->old_cursor_x != ctx->cursor_x || ctx->old_cursor_y != ctx->cursor_y) || _ctx->cursor_enabled == false) {
		if (ctx->old_cursor_x < _ctx->cols && ctx->old_cursor_y < _ctx->rows) {
			ctx->plot_char(_ctx, &ctx->grid[ctx->old_cursor_x + ctx->old_cursor_y * _ctx->cols], ctx->old_cursor_x, ctx->old_cursor_y);
		}
	}

	ctx->old_cursor_x = ctx->cursor_x;
	ctx->old_cursor_y = ctx->cursor_y;

	ctx->queue_i = 0;
}

static void flanterm_fb_raw_putchar(struct flanterm_context *_ctx, uint8_t c) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	if (ctx->cursor_x >= _ctx->cols && (ctx->cursor_y < _ctx->scroll_bottom_margin - 1 || _ctx->scroll_enabled)) {
		ctx->cursor_x = 0;
		ctx->cursor_y++;
		if (ctx->cursor_y == _ctx->scroll_bottom_margin) {
			ctx->cursor_y--;
			flanterm_fb_scroll(_ctx);
		}
		if (ctx->cursor_y >= _ctx->cols) {
			ctx->cursor_y = _ctx->cols - 1;
		}
	}

	struct flanterm_fb_char ch;
	ch.c = c;
	ch.fg = ctx->text_fg;
	ch.bg = ctx->text_bg;
	push_to_queue(_ctx, &ch, ctx->cursor_x++, ctx->cursor_y);
}

static void flanterm_fb_full_refresh(struct flanterm_context *_ctx) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	uint32_t default_bg = ctx->default_bg;

	for (size_t y = 0; y < ctx->height; y++) {
		for (size_t x = 0; x < ctx->width; x++) {
			if (ctx->canvas != NULL) {
				ctx->framebuffer[y * (ctx->pitch / sizeof(uint32_t)) + x] = ctx->canvas[y * ctx->width + x];
			} else {
				ctx->framebuffer[y * (ctx->pitch / sizeof(uint32_t)) + x] = default_bg;
			}
		}
	}

	for (size_t i = 0; i < (size_t) _ctx->rows * _ctx->cols; i++) {
		size_t x = i % _ctx->cols;
		size_t y = i / _ctx->cols;

		ctx->plot_char(_ctx, &ctx->grid[i], x, y);
	}

	if (_ctx->cursor_enabled) {
		draw_cursor(_ctx);
	}
}

static void flanterm_fb_deinit(struct flanterm_context *_ctx, void (*_free)(void *, size_t)) {
	struct flanterm_fb_context *ctx = (void *) _ctx;

	if (_free == NULL) {
		return;
	}

	_free(ctx->font_bits, ctx->font_bits_size);
	_free(ctx->font_bool, ctx->font_bool_size);
	_free(ctx->grid, ctx->grid_size);
	_free(ctx->queue, ctx->queue_size);
	_free(ctx->map, ctx->map_size);

	if (ctx->canvas != NULL) {
		_free(ctx->canvas, ctx->canvas_size);
	}

	_free(ctx, sizeof(struct flanterm_fb_context));
}

struct flanterm_context *flanterm_fb_init(
	void *(*_malloc)(size_t),
	void (*_free)(void *, size_t),
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
) {
	if (font_scale_x == 0 || font_scale_y == 0) {
		font_scale_x = 1;
		font_scale_y = 1;
		if (width >= (1920 + 1920 / 3) && height >= (1080 + 1080 / 3)) {
			font_scale_x = 2;
			font_scale_y = 2;
		}
		if (width >= (3840 + 3840 / 3) && height >= (2160 + 2160 / 3)) {
			font_scale_x = 4;
			font_scale_y = 4;
		}
	}

	if (red_mask_size < 8 || red_mask_size != green_mask_size || red_mask_size != blue_mask_size) {
		return NULL;
	}

	if (_malloc == NULL) {
		return NULL;
	}

	struct flanterm_fb_context *ctx = NULL;
	ctx = _malloc(sizeof(struct flanterm_fb_context));
	if (ctx == NULL) {
		goto fail;
	}

	struct flanterm_context *_ctx = (void *) ctx;
	memset(ctx, 0, sizeof(struct flanterm_fb_context));

	ctx->red_mask_size = red_mask_size;
	ctx->red_mask_shift = red_mask_shift + (red_mask_size - 8);
	ctx->green_mask_size = green_mask_size;
	ctx->green_mask_shift = green_mask_shift + (green_mask_size - 8);
	ctx->blue_mask_size = blue_mask_size;
	ctx->blue_mask_shift = blue_mask_shift + (blue_mask_size - 8);

	if (ansi_colours != NULL) {
		for (size_t i = 0; i < 8; i++) {
			ctx->ansi_colours[i] = convert_colour(_ctx, ansi_colours[i]);
		}
	} else {
		ctx->ansi_colours[0] = convert_colour(_ctx, 0x00000000); // black
		ctx->ansi_colours[1] = convert_colour(_ctx, 0x00aa0000); // red
		ctx->ansi_colours[2] = convert_colour(_ctx, 0x0000aa00); // green
		ctx->ansi_colours[3] = convert_colour(_ctx, 0x00aa5500); // brown
		ctx->ansi_colours[4] = convert_colour(_ctx, 0x000000aa); // blue
		ctx->ansi_colours[5] = convert_colour(_ctx, 0x00aa00aa); // magenta
		ctx->ansi_colours[6] = convert_colour(_ctx, 0x0000aaaa); // cyan
		ctx->ansi_colours[7] = convert_colour(_ctx, 0x00aaaaaa); // grey
	}

	if (ansi_bright_colours != NULL) {
		for (size_t i = 0; i < 8; i++) {
			ctx->ansi_bright_colours[i] = convert_colour(_ctx, ansi_bright_colours[i]);
		}
	} else {
		ctx->ansi_bright_colours[0] = convert_colour(_ctx, 0x00555555); // black
		ctx->ansi_bright_colours[1] = convert_colour(_ctx, 0x00ff5555); // red
		ctx->ansi_bright_colours[2] = convert_colour(_ctx, 0x0055ff55); // green
		ctx->ansi_bright_colours[3] = convert_colour(_ctx, 0x00ffff55); // brown
		ctx->ansi_bright_colours[4] = convert_colour(_ctx, 0x005555ff); // blue
		ctx->ansi_bright_colours[5] = convert_colour(_ctx, 0x00ff55ff); // magenta
		ctx->ansi_bright_colours[6] = convert_colour(_ctx, 0x0055ffff); // cyan
		ctx->ansi_bright_colours[7] = convert_colour(_ctx, 0x00ffffff); // grey
	}

	if (default_bg != NULL) {
		ctx->default_bg = convert_colour(_ctx, *default_bg);
	} else {
		ctx->default_bg = 0x00000000; // background (black)
	}

	if (default_fg != NULL) {
		ctx->default_fg = convert_colour(_ctx, *default_fg);
	} else {
		ctx->default_fg = convert_colour(_ctx, 0x00aaaaaa); // foreground (grey)
	}

	if (default_bg_bright != NULL) {
		ctx->default_bg_bright = convert_colour(_ctx, *default_bg_bright);
	} else {
		ctx->default_bg_bright = convert_colour(_ctx, 0x00555555); // background (black)
	}

	if (default_fg_bright != NULL) {
		ctx->default_fg_bright = convert_colour(_ctx, *default_fg_bright);
	} else {
		ctx->default_fg_bright = convert_colour(_ctx, 0x00ffffff); // foreground (grey)
	}

	ctx->text_fg = ctx->default_fg;
	ctx->text_bg = 0xffffffff;

	ctx->framebuffer = (void *) framebuffer;
	ctx->width = width;
	ctx->height = height;
	ctx->pitch = pitch;

	const size_t FONT_BYTES = ((font_width * font_height * FLANTERM_FB_FONT_GLYPHS) / 8);

	ctx->font_width = font_width;
	ctx->font_height = font_height;
	ctx->font_bits_size = FONT_BYTES;
	ctx->font_bits = _malloc(ctx->font_bits_size);
	if (ctx->font_bits == NULL) {
		dprintf("ctx->font_bits == null\n");
		goto fail;
	}
	memcpy(ctx->font_bits, font, ctx->font_bits_size);

	ctx->font_width += font_spacing;

	ctx->font_bool_size = FLANTERM_FB_FONT_GLYPHS * font_height * ctx->font_width * sizeof(bool);
	ctx->font_bool = _malloc(ctx->font_bool_size);
	if (ctx->font_bool == NULL) {
		dprintf("ctx->font_bool == null\n");
		goto fail;
	}

	for (size_t i = 0; i < FLANTERM_FB_FONT_GLYPHS; i++) {
		uint8_t *glyph = &ctx->font_bits[i * font_height];

		for (size_t y = 0; y < font_height; y++) {
			// NOTE: the characters in VGA fonts are always one byte wide.
			// 9 dot wide fonts have 8 dots and one empty column, except
			// characters 0xC0-0xDF replicate column 9.
			for (size_t x = 0; x < 8; x++) {
				size_t offset = i * font_height * ctx->font_width + y * ctx->font_width + x;

				if ((glyph[y] & (0x80 >> x))) {
					ctx->font_bool[offset] = true;
				} else {
					ctx->font_bool[offset] = false;
				}
			}
			// fill columns above 8 like VGA Line Graphics Mode does
			for (size_t x = 8; x < ctx->font_width; x++) {
				size_t offset = i * font_height * ctx->font_width + y * ctx->font_width + x;

				if (i >= 0xc0 && i <= 0xdf) {
					ctx->font_bool[offset] = (glyph[y] & 1);
				} else {
					ctx->font_bool[offset] = false;
				}
			}
		}
	}

	ctx->font_scale_x = font_scale_x;
	ctx->font_scale_y = font_scale_y;

	ctx->glyph_width = ctx->font_width * font_scale_x;
	ctx->glyph_height = font_height * font_scale_y;

	_ctx->cols = (ctx->width - margin * 2) / ctx->glyph_width;
	_ctx->rows = (ctx->height - margin * 2) / ctx->glyph_height;

	ctx->offset_x = margin + ((ctx->width - margin * 2) % ctx->glyph_width) / 2;
	ctx->offset_y = margin + ((ctx->height - margin * 2) % ctx->glyph_height) / 2;

	ctx->grid_size = _ctx->rows * _ctx->cols * sizeof(struct flanterm_fb_char);
	ctx->grid = _malloc(ctx->grid_size);
	if (ctx->grid == NULL) {
		dprintf("ctx->grid == null\n");
		goto fail;
	}
	for (size_t i = 0; i < _ctx->rows * _ctx->cols; i++) {
		ctx->grid[i].c = ' ';
		ctx->grid[i].fg = ctx->text_fg;
		ctx->grid[i].bg = ctx->text_bg;
	}

	ctx->queue_size = _ctx->rows * _ctx->cols * sizeof(struct flanterm_fb_queue_item);
	dprintf("rows=%lu cols=%lu\n", _ctx->rows, _ctx->cols);
	ctx->queue = _malloc(ctx->queue_size);
	if (ctx->queue == NULL) {
		dprintf("ctx->queue == null\n");
		goto fail;
	}
	ctx->queue_i = 0;
	memset(ctx->queue, 0, ctx->queue_size);

	ctx->map_size = _ctx->rows * _ctx->cols * sizeof(struct flanterm_fb_queue_item *);
	ctx->map = _malloc(ctx->map_size);
	if (ctx->map == NULL) {
		dprintf("ctx->map == null\n");
		goto fail;
	}
	memset(ctx->map, 0, ctx->map_size);

	if (canvas != NULL) {
		ctx->canvas_size = ctx->width * ctx->height * sizeof(uint32_t);
		ctx->canvas = _malloc(ctx->canvas_size);
		if (ctx->canvas == NULL) {
			goto fail;
		}
		for (size_t i = 0; i < ctx->width * ctx->height; i++) {
			ctx->canvas[i] = convert_colour(_ctx, canvas[i]);
		}
	}

	if (font_scale_x == 1 && font_scale_y == 1) {
		if (canvas == NULL) {
			ctx->plot_char = plot_char_unscaled_uncanvas;
		} else {
			ctx->plot_char = plot_char_unscaled_canvas;
		}
	} else {
		if (canvas == NULL) {
			ctx->plot_char = plot_char_scaled_uncanvas;
		} else {
			ctx->plot_char = plot_char_scaled_canvas;
		}
	}

	_ctx->raw_putchar = flanterm_fb_raw_putchar;
	_ctx->clear = flanterm_fb_clear;
	_ctx->set_cursor_pos = flanterm_fb_set_cursor_pos;
	_ctx->get_cursor_pos = flanterm_fb_get_cursor_pos;
	_ctx->set_text_fg = flanterm_fb_set_text_fg;
	_ctx->set_text_bg = flanterm_fb_set_text_bg;
	_ctx->set_text_fg_bright = flanterm_fb_set_text_fg_bright;
	_ctx->set_text_bg_bright = flanterm_fb_set_text_bg_bright;
	_ctx->set_text_fg_rgb = flanterm_fb_set_text_fg_rgb;
	_ctx->set_text_bg_rgb = flanterm_fb_set_text_bg_rgb;
	_ctx->set_text_fg_default = flanterm_fb_set_text_fg_default;
	_ctx->set_text_bg_default = flanterm_fb_set_text_bg_default;
	_ctx->set_text_fg_default_bright = flanterm_fb_set_text_fg_default_bright;
	_ctx->set_text_bg_default_bright = flanterm_fb_set_text_bg_default_bright;
	_ctx->move_character = flanterm_fb_move_character;
	_ctx->scroll = flanterm_fb_scroll;
	_ctx->revscroll = flanterm_fb_revscroll;
	_ctx->swap_palette = flanterm_fb_swap_palette;
	_ctx->save_state = flanterm_fb_save_state;
	_ctx->restore_state = flanterm_fb_restore_state;
	_ctx->double_buffer_flush = flanterm_fb_double_buffer_flush;
	_ctx->full_refresh = flanterm_fb_full_refresh;
	_ctx->deinit = flanterm_fb_deinit;

	flanterm_context_reinit(_ctx);
	flanterm_fb_full_refresh(_ctx);

	return _ctx;

	fail:
	if (ctx == NULL) {
		return NULL;
	}

	if (_free == NULL) {
		return NULL;
	}

	if (ctx->canvas != NULL) {
		_free(ctx->canvas, ctx->canvas_size);
	}
	if (ctx->map != NULL) {
		_free(ctx->map, ctx->map_size);
	}
	if (ctx->queue != NULL) {
		_free(ctx->queue, ctx->queue_size);
	}
	if (ctx->grid != NULL) {
		_free(ctx->grid, ctx->grid_size);
	}
	if (ctx->font_bool != NULL) {
		_free(ctx->font_bool, ctx->font_bool_size);
	}
	if (ctx->font_bits != NULL) {
		_free(ctx->font_bits, ctx->font_bits_size);
	}
	if (ctx != NULL) {
		_free(ctx, sizeof(struct flanterm_fb_context));
	}

	return NULL;
}


/* Local helper to rebuild one glyph into font_bool, from ctx->font_bits. */
static void rebuild_one(struct flanterm_fb_context *ctx_local, size_t g)
{
	size_t fh_local = ctx_local->font_height;
	size_t fw_local = ctx_local->font_width;

	/* Source: one byte per row, VGA-style. */
	uint8_t *src = &ctx_local->font_bits[g * fh_local];

	for (size_t y = 0; y < fh_local; y++) {
		/* Destination row start in bool atlas. */
		bool *dst_row = &ctx_local->font_bool[g * fh_local * fw_local + y * fw_local];

		/* Columns 0..7 come from the bitplane (MSB-first). */
		uint8_t row_bits = src[y];
		for (size_t x = 0; x < 8 && x < fw_local; x++) {
			dst_row[x] = (row_bits & (uint8_t)(0x80u >> x)) ? true : false;
		}

		/* Columns >= 8 follow VGA line-graphics rule: 0xC0–0xDF copies bit 0 into col 8;
		 * all other glyphs clear extra columns. Remaining spacing columns are cleared.
		 */
		for (size_t x = 8; x < fw_local; x++) {
			if (g >= 0xC0 && g <= 0xDF) {
				dst_row[x] = (row_bits & 0x01u) ? true : false;
			} else {
				dst_row[x] = false;
			}
		}
	}
}

void flanterm_fb_update_font(struct flanterm_context *_ctx, int glyph, const uint8_t *bitmap)
{
	struct flanterm_fb_context *ctx = (void *)_ctx;

	size_t glyph_count = FLANTERM_FB_FONT_GLYPHS;
	size_t fh = ctx->font_height;          /* source rows (bytes per row) */

	/* Sanity: nothing to do if we have no buffers. */
	if (ctx == NULL || ctx->font_bits == NULL || ctx->font_bool == NULL) {
		return;
	}

	/* Path A: rebuild whole atlas. */
	if (glyph < 0) {
		for (size_t g = 0; g < glyph_count; g++) {
			rebuild_one(ctx, g);
			ft_mark_redefined(g);
		}
		return;
	}

	/* Path B: single-glyph update, if index valid. */
	if ((size_t)glyph >= glyph_count) {
		return;
	}

	/* If caller supplied a new bitmap, copy it into font_bits first. */
	if (bitmap != NULL) {
		uint8_t *dst = &ctx->font_bits[(size_t)glyph * fh];
		for (size_t y = 0; y < fh; y++) {
			dst[y] = bitmap[y];
		}
	}

	/* Rebuild only this glyph into font_bool. */
	rebuild_one(ctx, glyph);
	ft_mark_redefined(glyph);
}

/* Draw a single glyph at an arbitrary pixel position, bypassing grid/queue.
 * Fully clipped for negative x/y and partial visibility.
 */
void flanterm_fb_draw_glyph_px(struct flanterm_context *_ctx, uint8_t glyph, int32_t px, int32_t py, uint32_t fg_rgb, uint32_t bg_rgb, bool transparent_bg) {
	struct flanterm_fb_context *ctx = (void *)_ctx;

	uint32_t fg = convert_colour(_ctx, fg_rgb);
	uint32_t bg = convert_colour(_ctx, bg_rgb);

	int32_t fw = (int32_t)ctx->font_width;
	int32_t fh = (int32_t)ctx->font_height;
	int32_t sx = (int32_t)ctx->font_scale_x;
	int32_t sy = (int32_t)ctx->font_scale_y;

	int32_t gw = fw * sx;  /* glyph width in pixels */
	int32_t gh = fh * sy;  /* glyph height in pixels */

	if (py < ft_min_y) {
		ft_min_y = py;
	}
	if (py + gh > ft_max_y) {
		ft_max_y = py + gh;
	}

	int32_t max_x = (int32_t)ctx->width;
	int32_t max_y = (int32_t)ctx->height;

	/* Trivial reject if entirely off-screen. Use 64-bit to avoid mul overflow paranoia. */
	if ((int64_t)px >= (int64_t)max_x || (int64_t)py >= (int64_t)max_y) {
		return;
	}
	if ((int64_t)px + (int64_t)gw <= 0 || (int64_t)py + (int64_t)gh <= 0) {
		return;
	}

	bool *glyph_bits = &ctx->font_bool[(size_t)glyph * (size_t)fh * (size_t)fw];

	for (int32_t gy = 0; gy < gh; gy++) {
		int32_t y = py + gy;
		if (y < 0) {
			continue;
		}
		if (y >= max_y) {
			break;
		}

		int32_t fy = gy / sy;
		volatile uint32_t *fb_line = ctx->framebuffer + (size_t)y * (ctx->pitch / sizeof(uint32_t));

		for (int32_t fx = 0; fx < fw; fx++) {
			bool bit = glyph_bits[fy * fw + fx];

			/* Horizontal scaling with full left/right clipping */
			int32_t x0 = px + fx * sx;
			for (int32_t rep = 0; rep < sx; rep++) {
				int32_t x = x0 + rep;
				if (x < 0) {
					continue;
				}
				if (x >= max_x) {
					break;
				}
				if (bit) {
					fb_line[x] = fg;
				} else if (!transparent_bg) {
					fb_line[x] = bg;
				}
			}
		}
	}
}

void flanterm_fb_draw_text_px(struct flanterm_context *_ctx, const char *s, int32_t px, int32_t py, uint32_t fg_rgb, uint32_t bg_rgb, bool transparent_bg) {
	struct flanterm_fb_context *ctx = (void *)_ctx;

	int32_t pen_x = px;
	int32_t pen_y = py;

	int32_t adv_x = (int32_t)ctx->font_width * (int32_t)ctx->font_scale_x;
	int32_t adv_y = (int32_t)ctx->font_height * (int32_t)ctx->font_scale_y;

	for (const unsigned char *p = (const unsigned char *)s; *p != '\0'; p++) {
		unsigned char ch = *p;

		if (ch == '\n') {
			pen_x = px;
			pen_y += adv_y;
			continue;
		}

		flanterm_fb_draw_glyph_px(_ctx, ch, pen_x, pen_y, fg_rgb, bg_rgb, transparent_bg);

		/* Advance; safe even if we just drew off-screen thanks to per-glyph clipping */
		pen_x += adv_x;

		/* Optional early-stop if we have moved past the right edge by a full glyph */
		if ((int64_t)pen_x >= (int64_t)ctx->width && pen_y >= (int32_t)ctx->height) {
			break;
		}
	}
}
