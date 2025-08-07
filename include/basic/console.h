#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/* Console functions */
int64_t basic_get_text_max_x(struct basic_ctx* ctx);
int64_t basic_get_text_max_y(struct basic_ctx* ctx);
int64_t basic_get_text_cur_x(struct basic_ctx* ctx);
int64_t basic_get_text_cur_y(struct basic_ctx* ctx);
int64_t basic_ctrlkey(struct basic_ctx* ctx);
int64_t basic_shiftkey(struct basic_ctx* ctx);
int64_t basic_altkey(struct basic_ctx* ctx);
int64_t basic_capslock(struct basic_ctx* ctx);
void input_statement(struct basic_ctx* ctx);
void kget_statement(struct basic_ctx* ctx);
void cls_statement(struct basic_ctx* ctx);
void gotoxy_statement(struct basic_ctx* ctx);
void print_statement(struct basic_ctx* ctx);
void colour_statement(struct basic_ctx* ctx, int tok);
void background_statement(struct basic_ctx* ctx);
void keymap_statement(struct basic_ctx* ctx);
