/**
 * @file basic/double_expression.c
 * @brief BASIC double/real expression functions
 */
#include <kernel.h>
#include "basic/unified_expression.h"

void double_expr(struct basic_ctx* ctx, double* res) {
	up_double_expr_strict(ctx, res);
}