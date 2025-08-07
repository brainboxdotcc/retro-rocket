#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/*
 * Integer expression evaluation
 */
int64_t expr(struct basic_ctx* ctx);
int64_t relation(struct basic_ctx* ctx);

/*
 * Real (double) expression evalutation
 */
void double_expr(struct basic_ctx* ctx, double* res);
void double_relation(struct basic_ctx* ctx, double* res);

/*
 * String expression evaluation
 */
const char* str_expr(struct basic_ctx* ctx);
int64_t str_relation(struct basic_ctx* ctx);
