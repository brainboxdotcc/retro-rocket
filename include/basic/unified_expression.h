#pragma once
#include <kernel.h>

/**
 * @file unified_expression.h
 * @brief Typed expression and conditional entry points for the unified parser.
 *
 * @details
 * Single typed recursive–descent evaluator supporting INT / REAL / STRING.
 * Provides drop-in entry points for conditionals and strict wrappers for
 * integer, real, and string expressions.
 *
 * ### Tokeniser interaction
 * - **Parentheses**: content inside `(` … `)` is parsed as a relation-aware
 *   expression so forms such as `(in<128)` are valid in IF/WHILE.
 * - **String literals**: obtained via `tokenizer_string(...)`. No subsequent
 *   `accept(STRING)` is performed; the tokeniser already advances to lookahead.
 * - **Variables and built-ins**:
 *   - Names ending with `$` → STRING.
 *   - Names ending with `#` → REAL.
 *   - Unsuffixed names → INT, *except* for a temporary whitelist of known
 *     real-returning built-ins (implemented in the `.c` file):
 *     `ACS, ASN, ATAN, ATAN2, CEIL, COS, DEG, EXP, FMOD, GETVARR, LOG, POW,
 *      RAD, REALVAL, ROUND, SIN, SQRT, TAN`.
 *
 * ### Operators
 * - **Arithmetic**: `+ - * / MOD`
 *   - Numeric arithmetic; `+` concatenates when both operands are strings.
 *   - Mixed string/numeric on `+` or `-` is an error. `MOD` requires integers.
 *   - Division by zero emits a kernel-style error. INT+REAL promotes to REAL.
 * - **Bitwise**: `AND OR EOR`
 *   - Numeric only; REAL operands are cast to `int64_t`; strings are an error.
 * - **Relational**: `< > = <= >= <>`
 *   - Numeric comparisons promote INT↔REAL. String comparisons use `strcmp`.
 *   - Result is INT 0/1.
 * - **Boolean precedence (conditionals)**: `NOT` > `AND` > `OR`, left-associative.
 *
 * ### Consumption and stop conditions
 * - `up_conditional()` stops before `THEN` / `NEWLINE` / `EOF` / `')'`. `THEN`
 *   is not consumed; statement parsers should continue to `accept(THEN, ctx)`.
 *
 * ### String lifetime
 * - String results are duplicated with the GC (`gc_strdup`) to match existing
 *   `str_expr` behaviour.
 *
 * @note The temporary real-builtin whitelist can be replaced by a descriptor
 *       table (name → return type) without API changes.
 */

/**
 * @brief Evaluate a BASIC conditional (boolean) expression.
 *
 * @par Grammar (informal)
 * @code
 *   conditional := bool_term { (AND | OR) bool_term }
 *   bool_term   := [NOT]* relation
 *   relation    := value_expr { (< | > | =) [= | ><] value_expr }
 * @endcode
 *
 * @param ctx BASIC interpreter context.
 * @return `true` for non-zero truth value; `false` otherwise.
 *
 * @par Side effects
 * Advances the token stream over the condition. Does not consume `THEN`.
 * Emits kernel-style errors on type violations and division by zero.
 */
bool up_conditional(struct basic_ctx *ctx);

/**
 * @brief Evaluate a relational expression and return INT 0/1.
 *
 * @param ctx BASIC interpreter context.
 * @return `0` (false) or `1` (true).
 */
int64_t up_relation_i(struct basic_ctx *ctx);

/* ------------------------------------------------------------------------- */
/* Strict shims — drop-in replacements for legacy int/real/string expr APIs. */
/* ------------------------------------------------------------------------- */

/**
 * @brief Strict integer expression (replacement for `expr(ctx)`).
 *
 * Parses a numeric expression and returns INT. REAL results are narrowed to
 * `int64_t`. STRING results produce an error and return `0`.
 *
 * @param ctx BASIC interpreter context.
 * @return Evaluated integer value, or `0` on type error.
 */
int64_t up_int_expr_strict(struct basic_ctx *ctx);

/**
 * @brief Strict real (double) expression (replacement for `double_expr(ctx,&out)`).
 *
 * Parses a numeric expression and writes a REAL result to @p out. INT results
 * are widened to REAL. STRING results produce an error and write `0.0`.
 *
 * @param ctx BASIC interpreter context.
 * @param[out] out Destination for the evaluated real value.
 */
void up_double_expr_strict(struct basic_ctx *ctx, double *out);

/**
 * @brief Strict string expression (replacement for `str_expr(ctx)`).
 *
 * Parses a string expression and returns a GC-managed string pointer. Numeric
 * results produce an error and return the empty string `""`.
 *
 * @param ctx BASIC interpreter context.
 * @return Non-NULL GC-managed string pointer.
 */
const char* up_str_expr_strict(struct basic_ctx *ctx);
