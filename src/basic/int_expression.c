/**
 * @file basic/int_expression.c
 * @brief BASIC integer expression functions
 */
#include <kernel.h>
#include "basic/unified_expression.h"

int64_t expr(struct basic_ctx* ctx) {
	return up_int_expr_strict(ctx);
}