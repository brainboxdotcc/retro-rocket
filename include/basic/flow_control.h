#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

void def_statement(struct basic_ctx* ctx);
void proc_statement(struct basic_ctx* ctx);
void retproc_statement(struct basic_ctx* ctx);
void begin_comma_list(struct ub_proc_fn_def* def, struct basic_ctx* ctx);
uint8_t extract_comma_list(struct ub_proc_fn_def* def, struct basic_ctx* ctx);
void eq_statement(struct basic_ctx* ctx);
bool conditional(struct basic_ctx* ctx);
void else_statement(struct basic_ctx* ctx);
void if_statement(struct basic_ctx* ctx);
void gosub_statement(struct basic_ctx* ctx);
void return_statement(struct basic_ctx* ctx);
void next_statement(struct basic_ctx* ctx);
void for_statement(struct basic_ctx* ctx);
void repeat_statement(struct basic_ctx* ctx);
void until_statement(struct basic_ctx* ctx);
void endif_statement(struct basic_ctx* ctx);
void end_statement(struct basic_ctx* ctx);
void panic_statement(struct basic_ctx* ctx);
bool basic_in_eval(struct basic_ctx* ctx);
