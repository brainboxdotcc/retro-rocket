#pragma once

/**
 * @brief Maximum length of a string variable.
 *
 * This defines the maximum number of characters allowed for string variables in the program.
 */
#define MAX_STRINGLEN 1024

/**
 * @brief Maximum stack depth for GOSUB, PROC, and FN calls.
 *
 * This defines the maximum depth of nesting for procedure (`PROC`), function (`FN`),
 * and GOSUB calls in the program. This stack depth is used to track the return addresses
 * for each call level.
 */
#define MAX_CALL_STACK_DEPTH 255

/**
 * @brief Maximum stack depth for FOR...NEXT loops.
 *
 * This defines the maximum depth of nesting for `FOR...NEXT` loops in the program.
 * It controls how many loops can be nested before the stack is considered full.
 */
#define MAX_LOOP_STACK_DEPTH 255

/**
 * @brief Special line number where EVAL code is inserted dynamically.
 *
 * This constant represents the line number where the code inserted by an `EVAL`
 * statement will be placed. It is used to handle dynamic program changes during execution.
 */
#define EVAL_LINE	999999998

/**
 * @brief RETURN statement line after EVAL code.
 *
 * This line is used as the destination after the dynamically inserted `EVAL` code completes.
 * The program will return to this line number after executing the `EVAL` block.
 */
#define EVAL_END_LINE	999999999

/**
 * @brief Macro to convert a token into a string representation.
 *
 * This macro is used to turn a preprocessor token into a string.
 */
#define AUX(x) #x

/**
 * @brief Helper macro to stringify a token.
 *
 * This macro expands the `AUX` macro to convert an expression into a string.
 */
#define STRINGIFY(x) AUX(x)

/**
 * @brief Debugging break status.
 *
 * This flag indicates that the program is paused at a breakpoint for debugging.
 */
#define DEBUG_BREAK	1

/**
 * @brief Debugging step status.
 *
 * This flag indicates that the program is in step-by-step debugging mode.
 */
#define DEBUG_STEP	2

/**
 * @brief Debugging trace status.
 *
 * This flag indicates that the program is running in trace mode, where each instruction is logged.
 */
#define DEBUG_TRACE	4

/**
 * @brief Maximum number of sprites.
 *
 * This defines the maximum number of sprites that can be used in the program. A sprite represents
 * a graphical object that can be displayed and manipulated.
 */
#define MAX_SPRITES 1024

/**
 * @brief Begin parsing function parameters.
 *
 * This macro is used to initialize the context for parsing the parameters of a function
 * or procedure. It sets up necessary local variables like bracket depth, parameter pointer,
 * and the item start pointer.
 */
#define PARAMS_START \
	[[maybe_unused]] int itemtype = BIP_INT; \
	[[maybe_unused]] int64_t intval = 0; \
	[[maybe_unused]] double doubleval = 0; \
	[[maybe_unused]] char* strval = NULL; \
	[[maybe_unused]] char oldval = 0; \
	[[maybe_unused]] char oldct = 0; \
	[[maybe_unused]] char* oldptr = 0; \
	[[maybe_unused]] char const* oldnextptr = NULL; \
	[[maybe_unused]] int gotone = 0; \
	[[maybe_unused]] int bracket_depth = 0; \
	[[maybe_unused]] char const* item_begin = ctx->ptr;

/**
 * @brief Get a function parameter of a specific type.
 *
 * This macro is used to retrieve a function parameter of a specific type (string, double, integer)
 * from the input context, ensuring that the parsing is done correctly, respecting the parameter's type.
 * It processes the tokens, sets the corresponding parameter variable, and moves the context pointers.
 *
 * @param type The type of the parameter to get (string, double, integer, or variable).
 */
#define PARAMS_GET_ITEM(type) { gotone = 0; \
	while (!gotone) \
	{ \
		if (*ctx->ptr == '\n' || *ctx->ptr == 0) { \
			break; \
		} \
		if (*ctx->ptr == '(') { \
			bracket_depth++; \
			if (bracket_depth == 1) \
				item_begin = ctx->ptr + 1; \
		} \
       		else if (*ctx->ptr == ')') \
			bracket_depth--; \
		if ((*ctx->ptr == ',' && bracket_depth == 1) || (*ctx->ptr == ')' && bracket_depth == 0)) { \
			gotone = 1; \
			oldval = *ctx->ptr; \
			oldct = ctx->current_token; \
			oldptr = (char*)ctx->ptr; \
			oldnextptr = ctx->nextptr; \
			ctx->nextptr = item_begin; \
			ctx->ptr = item_begin; \
			ctx->current_token = get_next_token(ctx); \
			*oldptr = 0; \
			itemtype = type; \
			if (itemtype == BIP_STRING) { \
				strval = (char*)str_expr(ctx); \
			} else if (itemtype == BIP_DOUBLE) { \
				double_expr(ctx, &doubleval); \
			} else if (itemtype == BIP_VARIABLE) { \
				strval = (char*)tokenizer_variable_name(ctx); \
			} else { \
				intval = expr(ctx); \
			} \
			*oldptr = oldval; \
			ctx->ptr = oldptr; \
			ctx->nextptr = oldnextptr; \
			ctx->current_token = oldct; \
			item_begin = ctx->ptr + 1; \
			gotone = 1; \
		} \
		if (bracket_depth == 0 || *ctx->ptr == 0) { \
			ctx->nextptr = ctx->ptr; \
			ctx->current_token = get_next_token(ctx); \
			gotone = 1; \
		} \
		ctx->ptr++; \
	} \
}

/**
 * @brief End fetching function parameters.
 *
 * This macro is used to check if all function parameters have been parsed correctly.
 * It verifies that the closing bracket `)` is encountered and throws an error if there are
 * parameters remaining to be processed.
 *
 * @param NAME The name of the function or procedure being processed.
 * @param returnval The value to return in case of error.
 */
#define PARAMS_END(NAME, returnval) { \
	if (*(ctx->ptr - 1) != ')') { \
		tokenizer_error_print(ctx, "Invalid number of parameters for " NAME); \
		return returnval; \
	} \
}

/**
 * @brief End fetching function parameters for void return functions.
 *
 * Similar to `PARAMS_END` but used for functions that do not return a value.
 * It ensures that the function's parameter list is terminated properly.
 *
 * @param NAME The name of the function or procedure being processed.
 */
#define PARAMS_END_VOID(NAME) { \
	if (*(ctx->ptr - 1) != ')') { \
		tokenizer_error_print(ctx, "Invalid number of parameters for " NAME); \
		return; \
	} \
}

/**
 * @brief Simplified version of accept() for quick handling of token acceptance.
 *
 * This macro tries to accept a specific token in the context. If the token does not match,
 * it immediately returns from the current function, providing an efficient exit mechanism
 * for expected token checks.
 *
 * @param token The token to accept.
 * @param ctx The current interpreter context.
 */
#define accept_or_return(token, ctx) \
	if (!accept(token, ctx)) { \
		return; \
	}
