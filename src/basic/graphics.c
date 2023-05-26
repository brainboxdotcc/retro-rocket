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
			ctx->sprites[i] = kmalloc(sizeof(sprite_t));
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
		kfree(ctx->sprites[sprite_handle]);
		ctx->sprites[sprite_handle] = NULL;
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

typedef struct pngle_user_data_t {
	basic_ctx* ctx;
	int64_t sprite_handle;
} pngle_user_data_t;

sprite_t* get_sprite(struct basic_ctx* ctx, int64_t sprite_handle)
{
	if (sprite_handle >= 0 && sprite_handle < MAX_SPRITES && ctx->sprites[sprite_handle] != NULL) {
		return ctx->sprites[sprite_handle];
	}
	return NULL;
}

void loadsprite_statement(struct basic_ctx* ctx)
{
	accept_or_return(SPRITELOAD, ctx);
	const char* variable = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	const char* filename = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);

	const char* file = make_full_path(filename);
	int64_t sprite_handle = alloc_sprite(ctx);
	if (sprite_handle == -1) {
		tokenizer_error_print(ctx, "No more sprites available");
		return;
	}

	basic_set_int_variable(variable, sprite_handle, ctx, false, false);

	fs_directory_entry_t* f = fs_get_file_info(file);
	if (!f || fs_is_directory(file) || f->size == 0) {
		tokenizer_error_print(ctx, "Unable to open sprite file");
		return;
	}
	char* buf = kmalloc(f->size);
	fs_read_file(f, 0, f->size, (unsigned char*)buf);
	sprite_t* s = get_sprite(ctx, sprite_handle);
	int w, h, n;
	s->pixels = (uint32_t*)stbi_load_from_memory((unsigned char*)buf, f->size, &w, &h, &n, STBI_rgb_alpha);
	if (!s->pixels) {
		tokenizer_error_print(ctx, stbi_failure_reason());
		kfree(buf);
		free_sprite(ctx, sprite_handle);
		return;
	}
	s->width = w;
	s->height = h;
	dprintf("Width: %d Height: %d Comp: %d\n", w, h, n);
	kfree(buf);
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

int64_t basic_rgb(struct basic_ctx* ctx)
{
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

