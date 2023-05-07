#include <kernel.h>

const char* str_varfactor(struct basic_ctx* ctx)
{
	const char* r;
	if (*ctx->ptr == '"') {
		if (!tokenizer_string(ctx->string, sizeof(ctx->string), ctx)) {
			return "";
		}
		if (tokenizer_token(ctx) == CLOSEBRACKET) {
			accept(CLOSEBRACKET, ctx);
		 } else {
			accept(STRING, ctx);
		 }
		return ctx->string;
	} else {
		r = basic_get_string_variable(tokenizer_variable_name(ctx), ctx);
		if (tokenizer_token(ctx) == CLOSEBRACKET) {
			accept(CLOSEBRACKET, ctx);
		} else {
			accept(VARIABLE, ctx);
		}
	}
	return r;
}

const char* str_factor(struct basic_ctx* ctx)
{
	switch (tokenizer_token(ctx)) {
		case OPENBRACKET:
			accept(OPENBRACKET, ctx);
			const char* r = str_expr(ctx);
			accept(CLOSEBRACKET, ctx);
			return r;
		break;
		default:
			return str_varfactor(ctx);
		break;
	}
}

int64_t str_relation(struct basic_ctx* ctx)
{
	int r;

	const char* r1 = str_expr(ctx);
	int op = tokenizer_token(ctx);

	while (op == LESSTHAN || op == GREATERTHAN || op == EQUALS) {
		tokenizer_next(ctx);
		bool or_equal = false, not_equal = false;
		if (op == LESSTHAN || op == GREATERTHAN) {
			int secondary = tokenizer_token(ctx);
			if (secondary == EQUALS) {
				or_equal = true;
				tokenizer_next(ctx);
			}
			if (op == LESSTHAN && secondary == GREATERTHAN) {
				/* <>, not equals */
				op = EQUALS;
				not_equal = true;
				tokenizer_next(ctx);
			}
		}
		const char* r2 = str_expr(ctx);

		switch (op) {
			case LESSTHAN:
				r = or_equal ? (strcmp(r1, r2) <= 0) : (strcmp(r1, r2) < 0);
			break;
			case GREATERTHAN:
				r = or_equal ? (strcmp(r1, r2) >= 0) : (strcmp(r1, r2) > 0);
			break;
			case EQUALS:
				r = not_equal ? (strcmp(r1, r2) != 0) : (strcmp(r1, r2) == 0);
			break;
		}

		op = tokenizer_token(ctx);
	}

	return r;
}

const char* str_expr(struct basic_ctx* ctx)
{
	char tmp[MAX_STRINGLEN];
	char* t1 = (char*)str_factor(ctx);
	int op = tokenizer_token(ctx);

	strlcpy(tmp, t1, MAX_STRINGLEN);
	
	while (op == PLUS) {
		tokenizer_next(ctx);
		char* t2 = (char*)str_factor(ctx);
		switch (op) {
			case PLUS:
				strlcat(tmp, t2, MAX_STRINGLEN);
			break;
		}
		op = tokenizer_token(ctx);
	}
	return gc_strdup(tmp);
}
