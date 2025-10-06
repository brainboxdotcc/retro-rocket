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
 * - **Parentheses**: content inside `(` .. `)` is parsed as a relation-aware
 *   expression, so forms such as `(in<128)` are valid in IF/WHILE.
 * - **String literals**: obtained via `tokenizer_string(...)` and then
 *   `accept(STRING)`; the tokeniser produces the literal and we consume it.
 * - **Variables and built-ins**:
 *   - Names ending with `$` - STRING.
 *   - Names ending with `#` - REAL.
 *   - Unsuffixed names default to INT *unless* the core marks them as real
 *     built-ins (queried via the interpreter’s registry, e.g.
 *     `is_builtin_double_fn(name)`).
 *
 * ### Operators and precedence
 * - **Unary**: leading `+` / `-` bind **tighter** than `* / MOD`.
 *   Applies to numeric values only (e.g. `A--3`, `-(2+3)`, `-SIN(ANGLE#)`).
 * - **Arithmetic**: `+ - * / MOD`
 *   - Numeric arithmetic; `+` concatenates when **both** operands are strings.
 *   - Mixing string and numeric with `+`/`-` is an error. `MOD` requires INTs.
 *   - INT↔REAL promotes to REAL; division by zero reports a runtime error.
 * - **Relational**: `< > = <= >= <>`
 *   - Numeric comparisons promote INT↔REAL. String comparisons use `strcmp`.
 *   - Result is INT 0/1.
 * - **Boolean (conditionals)**: `NOT` > `AND` > `OR`, all left-associative.
 *
 * ### Consumption and stop conditions
 * - `up_conditional()` stops before `THEN` / `NEWLINE` / `EOF` / `')'`.
 *   The caller should `accept(THEN, ctx)` if required.
 * - `up_eval_value()` parses one value expression and **leaves** list
 *   separators (`,` / `;`), `THEN`, `NEWLINE`, and `EOF` untouched for the
 *   enclosing statement (e.g. `PRINT`, `INPUT`) to handle.
 *
 * ### String lifetime
 * - String results returned by expression evaluation are duplicated via the
 *   GC (`gc_strdup`) to match historic `str_expr` behaviour.
 */

#pragma once
#include <kernel.h>

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
/* Strict shims - drop-in replacements for legacy int/real/string expr APIs. */
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

/**
 * @brief Construct a typed value holding an integer.
 *
 * @param x  Signed 64-bit integer to store.
 * @return   up_value tagged as UP_INT with @p x in the integer field.
 *
 * @note No allocation is performed.
 */
up_value up_make_int(int64_t x);

/**
 * @brief Construct a typed value holding a real (double).
 *
 * @param x  Double to store.
 * @return   up_value tagged as UP_REAL with @p x in the real field.
 *
 * @note No allocation is performed.
 */
up_value up_make_real(double x);

/**
 * @brief Construct a typed value holding a string pointer.
 *
 * @param s  Pointer to NUL-terminated string to store (may be NULL).
 * @return   up_value tagged as UP_STR with @p s in the string field.
 *
 * @note The pointer is stored as-is; lifetime/ownership of @p s must be
 *       managed by the caller (typically GC-allocated in this interpreter).
 *       No copy is made here.
 */
up_value up_make_str(const char *s);

/**
 * @brief Evaluate a single value expression and return its typed result.
 *
 * Parses one expression using the unified evaluator and writes the result to
 * @p out without coercion (the kind will be UP_INT, UP_REAL, or UP_STR).
 *
 * @param ctx  BASIC interpreter context; @c ctx->ptr must point at the start
 *             of the expression to evaluate.
 * @param[out] out  Receives the typed result of the evaluation.
 *
 * @post On return, @c ctx->ptr is positioned at the first token not consumed
 *       by the expression. In particular, list separators (','/';'), THEN,
 *       NEWLINE, and ENDOFINPUT are left untouched for the caller to handle.
 *
 * @note Errors encountered during evaluation are reported via the existing
 *       error machinery (e.g. @c tokenizer_error_print) and reflected in
 *       @c ctx->errored; this function does not throw or longjmp.
 */
void up_eval_value(struct basic_ctx *ctx, up_value *out);
