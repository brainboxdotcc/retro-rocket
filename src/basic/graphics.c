/**
 * @file basic/graphics.c
 * @brief BASIC graphics drawing functions
 */
#include <kernel.h>
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
#define STBI_NO_HDR
#define STBI_NO_THREAD_LOCALS 
#define STBI_MALLOC(sz)           kmalloc(sz)
#define STBI_REALLOC(p,newsz)     krealloc(p,newsz)
#define STBI_FREE(p)              kfree(p)
#include <stb_image.h>

void __assert_fail(const char * assertion, const char * file, unsigned int line, const char * function)
{
	kprintf("%s at %s:%d (%s)\n", assertion, file, line, function);
	wait_forever();
	while(true);
}

int64_t alloc_sprite(struct basic_ctx* ctx)
{
	for (uint64_t i = 0; i < MAX_SPRITES; ++i) {
		if (ctx->sprites[i] == NULL) {
			ctx->sprites[i] = buddy_malloc(ctx->allocator, sizeof(sprite_t));
			ctx->sprites[i]->width = 0;
			ctx->sprites[i]->height = 0;
			ctx->sprites[i]->pixels = NULL;
			return i;
		}
	}
	return -1;
}

void free_sprite(struct basic_ctx* ctx, int64_t sprite_handle)
{
	if (sprite_handle >= 0 && sprite_handle < MAX_SPRITES && ctx->sprites[sprite_handle] != NULL) {
		if (ctx->sprites[sprite_handle]->pixels) {
			stbi_image_free(ctx->sprites[sprite_handle]->pixels);
		}
		buddy_free(ctx->allocator, ctx->sprites[sprite_handle]);
	}
}

void plot_sprite(struct basic_ctx* ctx, int64_t sprite_handle, int64_t draw_x, int64_t draw_y)
{
	if (sprite_handle >= 0 && sprite_handle < MAX_SPRITES && ctx->sprites[sprite_handle] != NULL && ctx->sprites[sprite_handle]->pixels != NULL) {
		sprite_t* s = ctx->sprites[sprite_handle];
		for (int64_t y = 0; y < s->height; ++y) {
			for (int64_t x = 0; x < s->width; ++x) {
				uint32_t pixel = ctx->sprites[sprite_handle]->pixels[y * s->width + x];
				if ((pixel & 0xff000000) == 0xff000000) {
					uint32_t a = (pixel & 0xff000000) >> 24;
					uint32_t r = (pixel & 0xff0000) >> 16;
					uint32_t g = (pixel & 0xff00) >> 8;
					uint32_t b = (pixel & 0xff);
					putpixel(x + draw_x, y + draw_y, (a << 24) | (b << 16) | (g << 8) | (r));
				}
			}
		}
	}
}

sprite_t* get_sprite(struct basic_ctx* ctx, int64_t sprite_handle)
{
	if (sprite_handle >= 0 && sprite_handle < MAX_SPRITES && ctx->sprites[sprite_handle] != NULL) {
		return ctx->sprites[sprite_handle];
	}
	return NULL;
}

void autoflip_statement(struct basic_ctx* ctx)
{
	accept_or_return(AUTOFLIP, ctx);
	int64_t enable = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	set_video_auto_flip(enable);
	ctx->claimed_flip = !enable;
}

void flip_statement(struct basic_ctx* ctx)
{
	accept_or_return(FLIP, ctx);
	accept_or_return(NEWLINE, ctx);
	if (video_flip_auto()) {
		tokenizer_error_print(ctx, "Video flipping is not set to manual mode");
	}
	rr_flip();
}

void loadsprite_statement(struct basic_ctx* ctx)
{
	accept_or_return(SPRITELOAD, ctx);
	const char* variable = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	const char* filename = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);

	const char* file = make_full_path(ctx, filename);
	int64_t sprite_handle = alloc_sprite(ctx);
	if (sprite_handle == -1) {
		tokenizer_error_print(ctx, "No more sprites available");
		return;
	}

	basic_set_int_variable(variable, sprite_handle, ctx, false, false);

	fs_directory_entry_t* f = fs_get_file_info(file);
	if (!f || fs_is_directory(file) || f->size == 0) {
		free_sprite(ctx, sprite_handle);
		tokenizer_error_printf(ctx, "Unable to open sprite file '%s'", file);
		return;
	}
	char* buf = buddy_malloc(ctx->allocator, f->size);
	if (!buf) {
		free_sprite(ctx, sprite_handle);
		tokenizer_error_printf(ctx, "Not enough memory to load sprite file '%s'", file);
		return;
	}
	fs_read_file(f, 0, f->size, (unsigned char*)buf);
	sprite_t* s = get_sprite(ctx, sprite_handle);
	int w, h, n;
	s->pixels = (uint32_t*)stbi_load_from_memory((unsigned char*)buf, f->size, &w, &h, &n, STBI_rgb_alpha);
	if (!s->pixels) {
		tokenizer_error_printf(ctx, "Error loading sprite file '%s': %s", file, stbi_failure_reason());
		buddy_free(ctx->allocator, buf);
		free_sprite(ctx, sprite_handle);
		return;
	}
	s->width = w;
	s->height = h;
	dprintf("Width: %d Height: %d Comp: %d\n", w, h, n);
	buddy_free(ctx->allocator, buf);
}

void freesprite_statement(struct basic_ctx* ctx)
{
	accept_or_return(SPRITEFREE, ctx);
	const char* variable = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(NEWLINE, ctx);
	free_sprite(ctx, basic_get_int_variable(variable, ctx));
}

void plot_statement(struct basic_ctx* ctx)
{
	accept_or_return(PLOT, ctx);
	const char* variable = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t x1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	plot_sprite(ctx, basic_get_int_variable(variable, ctx), x1, y1);
}

void gcol_statement(struct basic_ctx* ctx)
{
	accept_or_return(GCOL, ctx);
	ctx->graphics_colour = expr(ctx);
	//dprintf("New graphics color: %08X\n", ctx->graphics_colour);
	accept_or_return(NEWLINE, ctx);
}

void draw_line_statement(struct basic_ctx* ctx)
{
	accept_or_return(LINE, ctx);
	int64_t x1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	draw_line(x1, y1, x2, y2, ctx->graphics_colour);
}

void point_statement(struct basic_ctx* ctx)
{
	accept_or_return(POINT, ctx);
	int64_t x1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	putpixel(x1, y1, ctx->graphics_colour);
}

void triangle_statement(struct basic_ctx* ctx)
{
	accept_or_return(TRIANGLE, ctx);
	int64_t x1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x3 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y3 = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	draw_triangle(x1, y1, x2, y2, x3, y3, ctx->graphics_colour);
}

void rectangle_statement(struct basic_ctx* ctx)
{
	accept_or_return(RECTANGLE, ctx);
	int64_t x1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	draw_horizontal_rectangle(x1, y1, x2, y2, ctx->graphics_colour);
}

void circle_statement(struct basic_ctx* ctx)
{
	accept_or_return(CIRCLE, ctx);
	int64_t x = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t radius = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t filled = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	draw_circle(x, y, radius, filled, ctx->graphics_colour);
}

int64_t basic_rgb(struct basic_ctx* ctx) {
	int64_t r, g, b;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	r = intval;
	PARAMS_GET_ITEM(BIP_INT);
	g = intval;
	PARAMS_GET_ITEM(BIP_INT);
	b = intval;
	PARAMS_END("RGB", 0);
	return (uint32_t)(r << 16 | g << 8 | b);
}

/* Projective textured-quad blitter for Retro Rocket BASIC sprites.
   Maps the sprite's full texture (0..w, 0..h) to any convex screen quad.
   Preserves perspective via a true homography. Nearest-neighbour sampling.
   Only draws fully-opaque source pixels (alpha == 0xFF), matching plot_sprite().
*/
typedef struct {
	double x;
	double y;
} dpoint_t;

/* Integer fast path for axis-aligned rectangles (no perspective).
 * Returns true if it drew the quad; false means "use the general path"
 */
static bool plot_sprite_quad_axis_aligned_int(struct basic_ctx* ctx, int64_t sprite_handle, int64_t x0, int64_t y0, int64_t x1, int64_t y1, int64_t x2, int64_t y2, int64_t x3, int64_t y3) {

	sprite_t* s = ctx->sprites[sprite_handle];

	/* Two perpendicular, axis-aligned edges from q0 */
	bool u_h = (y1 == y0) && (x1 != x0);
	bool u_v = (x1 == x0) && (y1 != y0);
	bool v_h = (y3 == y0) && (x3 != x0);
	bool v_v = (x3 == x0) && (y3 != y0);

	bool perpendicular = (u_h && v_v) || (u_v && v_h);
	if (!perpendicular) {
		return false;
	}

	/* Rectangle identity for the fourth corner */
	if (x2 != (x1 + x3 - x0) || y2 != (y1 + y3 - y0)) {
		return false;
	}

	/* Integer bbox (no loops) */
	int minx = (x0 < x1) ? x0 : x1;
	if (x3 < minx) {
		minx = x3;
	}
	if (x2 < minx) {
		minx = x2;
	}

	int maxx = (x0 > x1) ? x0 : x1;
	if (x3 > maxx) {
		maxx = x3;
	}
	if (x2 > maxx) {
		maxx = x2;
	}

	int miny = (y0 < y1) ? y0 : y1;
	if (y3 < miny) {
		miny = y3;
	}
	if (y2 < miny) {
		miny = y2;
	}

	int maxy = (y0 > y1) ? y0 : y1;
	if (y3 > maxy) {
		maxy = y3;
	}
	if (y2 > maxy) {
		maxy = y2;
	}

	if (minx == maxx || miny == maxy) {
		return true; /* Degenerate rectangle: nothing to draw, but handled. */
	}

	/* Fixed-point 16.16 steps mapping full sprite (0..w,0..h). */
	const int64_t fp_shift = 16;
	const int64_t one_fp = 1 << fp_shift;

	int64_t w = s->width;
	int64_t h = s->height;

	int64_t dx_u = 0;
	int64_t dy_u = 0;
	int64_t dx_v = 0;
	int64_t dy_v = 0;

	if (u_h) {
		int64_t run = x1 - x0;
		if (run == 0) {
			return false;
		}
		dx_u = (w * one_fp) / run;
		dy_u = 0;
	} else {
		int64_t rise = y1 - y0;
		if (rise == 0) {
			return false;
		}
		dx_u = 0;
		dy_u = (w * one_fp) / rise;
	}

	if (v_v) {
		int64_t rise = y3 - y0;
		if (rise == 0) {
			return false;
		}
		dx_v = 0;
		dy_v = (h * one_fp) / rise;
	} else {
		int64_t run = x3 - x0;
		if (run == 0) {
			return false;
		}
		dx_v = (h * one_fp) / run;
		dy_v = 0;
	}

	/* Start at bbox top-left, expressed relative to q0. */
	int64_t u_row = (minx - x0) * dx_u + (miny - y0) * dy_u;
	int64_t v_row = (minx - x0) * dx_v + (miny - y0) * dy_v;

	for (int py = miny; py <= maxy; ++py) {
		int64_t u_fp = u_row;
		int64_t v_fp = v_row;

		for (int px = minx; px <= maxx; ++px) {
			int64_t ui = (u_fp >> fp_shift);
			int64_t vi = (v_fp >> fp_shift);

			if (ui < 0) {
				ui = 0;
			}
			if (ui >= w) {
				ui = w - 1;
			}
			if (vi < 0) {
				vi = 0;
			}
			if (vi >= h) {
				vi = h - 1;
			}

			uint32_t src = s->pixels[vi * w + ui];
			if ((src & 0xff000000) == 0xff000000) {
				uint32_t a = (src >> 24) & 0xff;
				uint32_t r = (src >> 16) & 0xff;
				uint32_t g = (src >> 8)  & 0xff;
				uint32_t b = (src)       & 0xff;
				putpixel(px, py, (a << 24) | (b << 16) | (g << 8) | r);
			}

			u_fp += dx_u;
			v_fp += dx_v;
		}

		u_row += dy_u;
		v_row += dy_v;
	}

	return true;
}


/* Solve an 8x8 linear system via Gauss–Jordan elimination.
 * 'a' is augmented as 8 rows of 9 doubles (last column is RHS).
 * Returns true on success
 */
static bool solve8(double a[8][9]) {
	for (int r = 0; r < 8; ++r) {
		int pivot = r;
		double best = a[pivot][r] < 0 ? -a[pivot][r] : a[pivot][r];
		for (int rr = r + 1; rr < 8; ++rr) {
			double mag = a[rr][r] < 0 ? -a[rr][r] : a[rr][r];
			if (mag > best) {
				best = mag;
				pivot = rr;
			}
		}
		if (best == 0.0) {
			return false;
		}
		if (pivot != r) {
			for (int c = r; c < 9; ++c) {
				double tmp = a[r][c];
				a[r][c] = a[pivot][c];
				a[pivot][c] = tmp;
			}
		}
		double div = a[r][r];
		for (int c = r; c < 9; ++c) {
			a[r][c] /= div;
		}
		for (int rr = 0; rr < 8; ++rr) {
			if (rr != r) {
				double f = a[rr][r];
				if (f != 0.0) {
					for (int c = r; c < 9; ++c) {
						a[rr][c] -= f * a[r][c];
					}
				}
			}
		}
	}
	return true;
}

/* Build screen->texture homography H such that:
 * [u v 1]^T ~ H * [x y 1]^T, with h33 fixed to 1.
 * screen[0..3] are the quad corners, tex are the corresponding texture corners
 * (typically (0,0),(w,0),(w,h),(0,h) in that order).
 * H_out is 3x3 row-major (length 9).
 */
static bool compute_homography(const dpoint_t screen[4], const dpoint_t tex[4], double H_out[9]) {
	double m[8][9];
	for (int i = 0; i < 4; ++i) {
		double x = screen[i].x;
		double y = screen[i].y;
		double u = tex[i].x;
		double v = tex[i].y;

		int r0 = 2 * i;
		int r1 = r0 + 1;

		/* Row for 'u' */
		m[r0][0] = x;   m[r0][1] = y;   m[r0][2] = 1.0;
		m[r0][3] = 0.0; m[r0][4] = 0.0; m[r0][5] = 0.0;
		m[r0][6] = -u * x; m[r0][7] = -u * y; m[r0][8] = u;

		/* Row for 'v' */
		m[r1][0] = 0.0; m[r1][1] = 0.0; m[r1][2] = 0.0;
		m[r1][3] = x;   m[r1][4] = y;   m[r1][5] = 1.0;
		m[r1][6] = -v * x; m[r1][7] = -v * y; m[r1][8] = v;
	}

	if (!solve8(m)) {
		return false;
	}

	/* Unknowns are: h11 h12 h13 h21 h22 h23 h31 h32, with h33 = 1 */
	H_out[0] = m[0][8];
	H_out[1] = m[1][8];
	H_out[2] = m[2][8];
	H_out[3] = m[3][8];
	H_out[4] = m[4][8];
	H_out[5] = m[5][8];
	H_out[6] = m[6][8];
	H_out[7] = m[7][8];
	H_out[8] = 1.0;

	return true;
}

/* Draw scanlines between a long edge (L0->L1) and a short edge (S0->S1)
   for integer rows y in [ceil(y_top), ceil(y_bottom) - 1].
   If long_is_left != 0, the long edge provides xL; otherwise it provides xR.
   Projective mapping: u = (H0*x + H1*y + H2) / (H6*x + H7*y + H8)
                       v = (H3*x + H4*y + H5) / (H6*x + H7*y + H8) */
static void draw_strip_projective(dpoint_t L0, dpoint_t L1, dpoint_t S0, dpoint_t S1, double y_top, double y_bottom, int long_is_left, const double H[9], const sprite_t* s, uint64_t framebuffer) {
	const double H0 = H[0], H1 = H[1], H2 = H[2];
	const double H3 = H[3], H4 = H[4], H5 = H[5];
	const double H6 = H[6], H7 = H[7], H8 = H[8];

	int y0 = (int)ceil(y_top);
	int y1 = (int)ceil(y_bottom);
	if (y1 <= y0) {
		return;
	}

	for (int y = y0; y < y1; ++y) {
		double yc = (double)y + 0.5;

		/* Interpolate x on each edge at this scanline (clamped) */
		double xa;
		{
			double dy = L1.y - L0.y;
			double t = (dy != 0.0) ? (yc - L0.y) / dy : 0.0;
			if (t < 0.0) t = 0.0;
			if (t > 1.0) t = 1.0;
			xa = L0.x + t * (L1.x - L0.x);
		}

		double xb;
		{
			double dy = S1.y - S0.y;
			double t = (dy != 0.0) ? (yc - S0.y) / dy : 0.0;
			if (t < 0.0) t = 0.0;
			if (t > 1.0) t = 1.0;
			xb = S0.x + t * (S1.x - S0.x);
		}

		double xL = long_is_left ? xa : xb;
		double xR = long_is_left ? xb : xa;

		/* Top-left fill rule: inclusive range [ceil(xL), ceil(xR) - 1] */
		int x0 = (int)ceil(xL);
		int x1 = (int)ceil(xR) - 1;
		if (x1 < x0) {
			continue;
		}

		/* Row bases for incremental projective evaluation */
		double u_base = H1 * yc + H2;
		double v_base = H4 * yc + H5;
		double w_base = H7 * yc + H8;

		double x_start = (double)x0 + 0.5;

		double u_num = H0 * x_start + u_base;
		double v_num = H3 * x_start + v_base;
		double w     = H6 * x_start + w_base;

		for (int x = x0; x <= x1; ++x) {
			if (w != 0.0) {
				double invw = 1.0 / w;
				double u = u_num * invw;
				double v = v_num * invw;

				if (u >= -0.5 && v >= -0.5 &&
				    u <= (double)s->width + 0.5 &&
				    v <= (double)s->height + 0.5) {

					int ui = (int)(u + 0.5);
					int vi = (int)(v + 0.5);

					if ((unsigned)ui < (unsigned)s->width &&
					    (unsigned)vi < (unsigned)s->height) {

						uint32_t src = s->pixels[vi * s->width + ui];
						if ((src & 0xff000000u) == 0xff000000u) {
							uint32_t a = (src >> 24) & 0xffu;
							uint32_t r = (src >> 16) & 0xffu;
							uint32_t g = (src >> 8)  & 0xffu;
							uint32_t b =  src        & 0xffu;
							volatile uint32_t* addr = (volatile uint32_t*)(framebuffer + pixel_address(x, y));
							*addr = (a << 24) | (b << 16) | (g << 8) | r;
						}
					}
				}
			}

			/* advance one pixel to the right */
			u_num += H0;
			v_num += H3;
			w     += H6;
		}
	}
}

/* Draw one triangle with spans and incremental projective mapping.
   H maps [x y 1]^T -> [u_num v_num w]^T, so u = u_num / w, v = v_num / w. */
/* Draw one triangle of the quad using the shared strip drawer twice (upper+lower). */
static void raster_tri_projective(const dpoint_t q[4], int ia, int ib, int ic, const double H[9], const sprite_t* s, uint64_t framebuffer) {
	/* sort vertices by y -> v0 (top), v1 (mid), v2 (bottom) */
	int i0 = ia;
	int i1 = ib;
	int i2 = ic;

	if (q[i1].y < q[i0].y) { int t = i0; i0 = i1; i1 = t; }
	if (q[i2].y < q[i1].y) { int t = i1; i1 = i2; i2 = t; }
	if (q[i1].y < q[i0].y) { int t = i0; i0 = i1; i1 = t; }

	dpoint_t v0 = q[i0];
	dpoint_t v1 = q[i1];
	dpoint_t v2 = q[i2];

	if (v2.y <= v0.y) {
		return; /* degenerate */
	}

	/* Decide which side v1 lies on relative to the long edge v0->v2 */
	double dxdy_long = (v2.x - v0.x) / (v2.y - v0.y);
	double x_on_long_at_y1 = v0.x + dxdy_long * (v1.y - v0.y);
	int long_is_left = (v1.x > x_on_long_at_y1); /* if v1 is right of long, long edge is left */

	/* Upper strip: between long edge (v0->v2) and short edge (v0->v1) over y in [v0.y, v1.y) */
	if (v1.y > v0.y) {
		draw_strip_projective(v0, v2, v0, v1, v0.y, v1.y, long_is_left, H, s, framebuffer);
	}

	/* Lower strip: between long edge (v0->v2) and short edge (v1->v2) over y in [v1.y, v2.y) */
	if (v2.y > v1.y) {
		draw_strip_projective(v0, v2, v1, v2, v1.y, v2.y, long_is_left, H, s, framebuffer);
	}
}

/* ---- Drop-in replacement using span rasterisation (no bbox walking) ---- */
void plot_sprite_quad(struct basic_ctx* ctx, int64_t sprite_handle, int64_t x0, int64_t y0, int64_t x1, int64_t y1, int64_t x2, int64_t y2, int64_t x3, int64_t y3) {
	if (sprite_handle < 0 || sprite_handle >= MAX_SPRITES || ctx->sprites[sprite_handle] == NULL || ctx->sprites[sprite_handle]->pixels == NULL) {
		return;
	}

	if (plot_sprite_quad_axis_aligned_int(ctx, sprite_handle, (int)x0, (int)y0, (int)x1, (int)y1, (int)x2, (int)y2, (int)x3, (int)y3)) {
		return;
	}

	sprite_t* s = ctx->sprites[sprite_handle];

	dpoint_t q[4] = {
		{ .x = (double)x0, .y = (double)y0 },
		{ .x = (double)x1, .y = (double)y1 },
		{ .x = (double)x2, .y = (double)y2 },
		{ .x = (double)x3, .y = (double)y3 },
	};

	dpoint_t tex[4] = {
		{ .x = 0.0,          .y = 0.0 },
		{ .x = (double)s->width,  .y = 0.0 },
		{ .x = (double)s->width,  .y = (double)s->height },
		{ .x = 0.0,          .y = (double)s->height },
	};

	/* Normalise winding: accept CW or CCW by flipping 1<->3 if area is negative. */
	double area =
		q[0].x * q[1].y - q[1].x * q[0].y +
		q[1].x * q[2].y - q[2].x * q[1].y +
		q[2].x * q[3].y - q[3].x * q[2].y +
		q[3].x * q[0].y - q[0].x * q[3].y;

	if (area < 0.0) {
		dpoint_t tmp = q[1]; q[1] = q[3]; q[3] = tmp;
		dpoint_t ttmp = tex[1]; tex[1] = tex[3]; tex[3] = ttmp;
	}

	double H[9];
	if (!compute_homography(q, tex, H)) {
		return;
	}

	/* For your dirty-area updater */
	double minx_d = q[0].x;
	double maxx_d = q[0].x;
	double miny_d = q[0].y;
	double maxy_d = q[0].y;
	for (int i = 1; i < 4; ++i) {
		if (q[i].x < minx_d) { minx_d = q[i].x; }
		if (q[i].x > maxx_d) { maxx_d = q[i].x; }
		if (q[i].y < miny_d) { miny_d = q[i].y; }
		if (q[i].y > maxy_d) { maxy_d = q[i].y; }
	}
	int miny = (int)floor(miny_d);
	int maxy = (int)ceil(maxy_d);

	uint64_t framebuffer = framebuffer_address();

	/* Draw only the interior via two triangles */
	raster_tri_projective(q, 0, 1, 2, H, s, framebuffer);
	raster_tri_projective(q, 0, 2, 3, H, s, framebuffer);

	set_video_dirty_area(miny, maxy);
}


void plotquad_statement(struct basic_ctx* ctx) {
	accept_or_return(PLOTQUAD, ctx);
	const char* variable = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t x0 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y0 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x3 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y3 = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	plot_sprite_quad(ctx, basic_get_int_variable(variable, ctx), x0, y0, x1, y1, x2, y2, x3, y3);
}

