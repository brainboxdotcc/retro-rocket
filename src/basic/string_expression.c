/**
 * @file basic/string_expression.c
 * @brief BASIC string expression functions
 */
#include <kernel.h>
#include "basic/unified_expression.h"

const char* str_expr(struct basic_ctx* ctx) {
	return up_str_expr_strict(ctx);
}