#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/* Graphics statements */
int64_t basic_rgb(struct basic_ctx* ctx);
void circle_statement(struct basic_ctx* ctx);
void triangle_statement(struct basic_ctx* ctx);
void point_statement(struct basic_ctx* ctx);
void draw_line_statement(struct basic_ctx* ctx);
void gcol_statement(struct basic_ctx* ctx);
void rectangle_statement(struct basic_ctx* ctx);
void loadsprite_statement(struct basic_ctx* ctx);
void freesprite_statement(struct basic_ctx* ctx);
void plot_statement(struct basic_ctx* ctx);
void free_sprite(struct basic_ctx* ctx, int64_t sprite_handle);
void autoflip_statement(struct basic_ctx* ctx);
void flip_statement(struct basic_ctx* ctx);

