#pragma once

/**
 * @brief Maximum length of a string variable
 */
#define MAX_STRINGLEN 1024

/**
 * @brief Maximum stack depth of GOSUB, PROC, FN
 */
#define MAX_CALL_STACK_DEPTH 255

/**
 * @brief Maximum stack depth of FOR...NEXT
 */
#define MAX_LOOP_STACK_DEPTH 255

/**
 * @brief Special line number where EVAL code is inserted dynamically
 */
#define EVAL_LINE	999999998
/**
 * @brief RETURN statement after EVAL code
 *
 */
#define EVAL_END_LINE	999999999

#define AUX(x) #x
#define STRINGIFY(x) AUX(x)

#define DEBUG_BREAK	1
#define DEBUG_STEP	2
#define DEBUG_TRACE	4

#define MAX_SPRITES 1024

/**
 * @brief Begin parsing function parameters
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
 * @brief Get a function parameter of type.
 * @param type Type of function parameter to get, fills one of the variables
 * strval, doubleval or intval.
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
 * @brief Ends fetching of function parameters, throwing an error if parameters still remain
 */
#define PARAMS_END(NAME, returnval) { \
	if (*(ctx->ptr - 1) != ')') { \
		tokenizer_error_print(ctx, "Invalid number of parameters for " NAME); \
		return returnval; \
	} \
}

#define PARAMS_END_VOID(NAME) { \
	if (*(ctx->ptr - 1) != ')') { \
		tokenizer_error_print(ctx, "Invalid number of parameters for " NAME); \
		return; \
	} \
}

#define accept_or_return(token, ctx) \
	if (!accept(token, ctx)) { \
		return; \
	}

