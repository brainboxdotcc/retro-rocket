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

/* Solve an 8x8 linear system via Gauss–Jordan elimination.
   'a' is augmented as 8 rows of 9 doubles (last column is RHS).
   Returns true on success. */
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
   [u v 1]^T ~ H * [x y 1]^T, with h33 fixed to 1.
   screen[0..3] are the quad corners, tex are the corresponding texture corners
   (typically (0,0),(w,0),(w,h),(0,h) in that order).
   H_out is 3x3 row-major (length 9). */
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

static inline bool point_in_tri(double px, double py, const dpoint_t a, const dpoint_t b, const dpoint_t c) {
	double abx = b.x - a.x;
	double aby = b.y - a.y;
	double bcx = c.x - b.x;
	double bcy = c.y - b.y;
	double cax = a.x - c.x;
	double cay = a.y - c.y;

	double apx = px - a.x;
	double apy = py - a.y;
	double bpx = px - b.x;
	double bpy = py - b.y;
	double cpx = px - c.x;
	double cpy = py - c.y;

	double c1 = abx * apy - aby * apx;
	double c2 = bcx * bpy - bcy * bpx;
	double c3 = cax * cpy - cay * cpx;

	bool all_nonneg = (c1 >= 0.0) && (c2 >= 0.0) && (c3 >= 0.0);
	bool all_nonpos = (c1 <= 0.0) && (c2 <= 0.0) && (c3 <= 0.0);

	return all_nonneg || all_nonpos;
}


static inline bool point_in_quad(double px, double py, const dpoint_t q[4]) {
	if (point_in_tri(px, py, q[0], q[1], q[2])) {
		return true;
	}
	if (point_in_tri(px, py, q[0], q[2], q[3])) {
		return true;
	}
	return false;
}

/* Sample nearest-neighbour from sprite 's' at floating (u,v).
   Returns packed ARGB as stored in s->pixels. Caller handles alpha policy. */
static inline uint32_t sample_sprite_nearest(const sprite_t* s, double u, double v) {
	if (u < 0.0) {
		u = 0.0;
	}
	if (v < 0.0) {
		v = 0.0;
	}
	if (u > (double)(s->width - 1)) {
		u = (double)(s->width - 1);
	}
	if (v > (double)(s->height - 1)) {
		v = (double)(s->height - 1);
	}

	int ui = (int)(u + 0.5);
	int vi = (int)(v + 0.5);

	return s->pixels[vi * s->width + ui];
}

void plot_sprite_quad(struct basic_ctx* ctx, int64_t sprite_handle, int64_t x0, int64_t y0, int64_t x1, int64_t y1, int64_t x2, int64_t y2, int64_t x3, int64_t y3) {
	if (sprite_handle < 0) {
		return;
	}
	if (sprite_handle >= MAX_SPRITES) {
		return;
	}
	if (ctx->sprites[sprite_handle] == NULL) {
		return;
	}
	if (ctx->sprites[sprite_handle]->pixels == NULL) {
		return;
	}

	sprite_t* s = ctx->sprites[sprite_handle];

	dpoint_t q[4];
	q[0].x = x0; q[0].y = y0;
	q[1].x = x1; q[1].y = y1;
	q[2].x = x2; q[2].y = y2;
	q[3].x = x3; q[3].y = y3;

	dpoint_t tex[4];
	tex[0].x = 0.0;
	tex[0].y = 0.0;
	tex[1].x = s->width;
	tex[1].y = 0.0;
	tex[2].x = s->width;
	tex[2].y = s->height;
	tex[3].x = 0.0;
	tex[3].y = s->height;

	/* Normalise winding: accept CW or CCW by flipping 1<->3 if area is negative. */
	double area =
		q[0].x * q[1].y - q[1].x * q[0].y +
		q[1].x * q[2].y - q[2].x * q[1].y +
		q[2].x * q[3].y - q[3].x * q[2].y +
		q[3].x * q[0].y - q[0].x * q[3].y;

	if (area < 0.0) {
		dpoint_t tmp = q[1];
		q[1] = q[3];
		q[3] = tmp;

		dpoint_t ttmp = tex[1];
		tex[1] = tex[3];
		tex[3] = ttmp;
	}

	double H[9];
	if (!compute_homography(q, tex, H)) {
		return;
	}

	double minx_d = q[0].x;
	double maxx_d = q[0].x;
	double miny_d = q[0].y;
	double maxy_d = q[0].y;

	for (int i = 1; i < 4; ++i) {
		if (q[i].x < minx_d) {
			minx_d = q[i].x;
		}
		if (q[i].x > maxx_d) {
			maxx_d = q[i].x;
		}
		if (q[i].y < miny_d) {
			miny_d = q[i].y;
		}
		if (q[i].y > maxy_d) {
			maxy_d = q[i].y;
		}
	}

	int minx = (int)floor(minx_d);
	int maxx = (int)ceil(maxx_d);
	int miny = (int)floor(miny_d);
	int maxy = (int)ceil(maxy_d);

	for (int py = miny; py <= maxy; ++py) {
		for (int px = minx; px <= maxx; ++px) {
			double cx = (double)px + 0.5;
			double cy = (double)py + 0.5;

			if (!point_in_quad(cx, cy, q)) {
				continue;
			}

			double denom = H[6] * cx + H[7] * cy + H[8];
			if (denom == 0.0) {
				continue;
			}

			double u = (H[0] * cx + H[1] * cy + H[2]) / denom;
			double v = (H[3] * cx + H[4] * cy + H[5]) / denom;

			if (u < -0.5 || v < -0.5 || u > (double)s->width + 0.5 || v > (double)s->height + 0.5) {
				continue;
			}

			uint32_t src = sample_sprite_nearest(s, u, v);

			if ((src & 0xff000000) == 0xff000000) {
				uint32_t a = (src >> 24) & 0xff;
				uint32_t r = (src >> 16) & 0xff;
				uint32_t g = (src >> 8)  & 0xff;
				uint32_t b = (src)       & 0xff;
				uint32_t out = (a << 24) | (b << 16) | (g << 8) | r;
				putpixel(px, py, out);
			}
		}
	}
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

