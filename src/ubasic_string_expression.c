#include <kernel.h>

const char* str_varfactor(struct ubasic_ctx* ctx)
{
	const char* r;
	if (*ctx->ptr == '"')
	{
		if (!tokenizer_string(ctx->string, sizeof(ctx->string), ctx)) {
			return "";
		}
		if (tokenizer_token(ctx) == CLOSEBRACKET)
			accept(CLOSEBRACKET, ctx);
		else
			accept(STRING, ctx);
		return ctx->string;
	}
	else
	{
		r = ubasic_get_string_variable(tokenizer_variable_name(ctx), ctx);
		if (tokenizer_token(ctx) == CLOSEBRACKET)
			accept(CLOSEBRACKET, ctx);
		else
			accept(VARIABLE, ctx);
	}
	return r;
}

const char* str_factor(struct ubasic_ctx* ctx)
{
	const char* r;

	switch(tokenizer_token(ctx))
	{
		case OPENBRACKET:
			accept(OPENBRACKET, ctx);
			r = str_expr(ctx);
			accept(CLOSEBRACKET, ctx);
		break;
		default:
			r = str_varfactor(ctx);
		break;
	}

	return r;
}

int64_t str_relation(struct ubasic_ctx* ctx)
{
	int r;

	const char* r1 = str_expr(ctx);
	int op = tokenizer_token(ctx);

	while (op == LESSTHAN || op == GREATERTHAN || op == EQUALS)
	{
		tokenizer_next(ctx);
		const char* r2 = str_expr(ctx);

		switch (op)
		{
			case LESSTHAN:
				r = (strcmp(r1, r2) < 0);
			break;
			case GREATERTHAN:
				r = (strcmp(r1, r2) > 0);
			break;
			case EQUALS:
				r = (strcmp(r1, r2) == 0);
			break;
		}

		op = tokenizer_token(ctx);
	}

	return r;
}

const char* str_expr(struct ubasic_ctx* ctx)
{
	char tmp[MAX_STRINGLEN];
	char* t1 = (char*)str_factor(ctx);
	int op = tokenizer_token(ctx);

	strlcpy(tmp, t1, MAX_STRINGLEN);
	
	while (op == PLUS)
	{
		tokenizer_next(ctx);
		char* t2 = (char*)str_factor(ctx);
		switch (op)
		{
			case PLUS:
				strlcat(tmp, t2, MAX_STRINGLEN);
			break;
		}
		op = tokenizer_token(ctx);
	}
	return gc_strdup(tmp);
}
