#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief Evaluates a full integer expression.
 *
 * This function parses and evaluates an integer expression from the BASIC program
 * based on the provided context. It supports all standard integer operations.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @return The result of the evaluated integer expression as a 64-bit signed integer.
 */
int64_t expr(struct basic_ctx* ctx);

/**
 * @brief Evaluates a relational integer expression.
 *
 * This function evaluates relational expressions (e.g., <, >, ==) in the
 * context of integer operations. It returns `1` for true and `0` for false.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @return The result of the relational expression as a 64-bit signed integer (`1` for true, `0` for false).
 */
int64_t relation(struct basic_ctx* ctx);


/**
 * @brief Evaluates a full real (double) expression.
 *
 * This function parses and evaluates a real (double) expression from the BASIC program
 * based on the provided context. It supports floating-point arithmetic operations.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @param res A pointer to a double where the result will be stored.
 */
void double_expr(struct basic_ctx* ctx, double* res);

/**
 * @brief Evaluates a relational real (double) expression.
 *
 * This function evaluates relational expressions (e.g., <, >, ==) for real numbers.
 * It returns `1` for true and `0` for false.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @param res A pointer to a double where the result will be stored.
 */
void double_relation(struct basic_ctx* ctx, double* res);


/**
 * @brief Evaluates a full string expression.
 *
 * This function parses and evaluates a string expression from the BASIC program
 * based on the provided context. It supports string concatenation and manipulation.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @return A pointer to the resulting string from the evaluated expression.
 */
const char* str_expr(struct basic_ctx* ctx);

/**
 * @brief Evaluates a relational string expression.
 *
 * This function evaluates relational expressions (e.g., ==, <, >) for strings.
 * It returns `1` for true and `0` for false.
 *
 * @param ctx A pointer to the current BASIC program context.
 * @return The result of the relational string expression as a 64-bit signed integer (`1` for true, `0` for false).
 */
int64_t str_relation(struct basic_ctx* ctx);
