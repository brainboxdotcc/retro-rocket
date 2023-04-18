#include <kernel.h>

const char* str_varfactor(struct ubasic_ctx* ctx)
{
	const char* r;
	if (*ctx->ptr == '"')
	{
		if (!tokenizer_string(ctx->string, sizeof(ctx->string), ctx)) {
			return "";
		}
		if (tokenizer_token(ctx) == RIGHTPAREN)
			accept(RIGHTPAREN, ctx);
		else
			accept(STRING, ctx);
		return ctx->string;
	}
	else
	{
		r = ubasic_get_string_variable(tokenizer_variable_name(ctx), ctx);
		if (tokenizer_token(ctx) == RIGHTPAREN)
			accept(RIGHTPAREN, ctx);
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
		case LEFTPAREN:
			accept(LEFTPAREN, ctx);
			r = str_expr(ctx);
			accept(RIGHTPAREN, ctx);
		break;
		default:
			r = str_varfactor(ctx);
		break;
	}

	return r;
}

int str_relation(struct ubasic_ctx* ctx)
{
	int op, r;

	const char* r1 = str_expr(ctx);
	op = tokenizer_token(ctx);

	while (op == LT || op == GT || op == EQ)
	{
		tokenizer_next(ctx);
		const char* r2 = str_expr(ctx);

		switch (op)
		{
			case LT:
				r = (strcmp(r1, r2) < 0);
			break;
			case GT:
				r = (strcmp(r1, r2) > 0);
			break;
			case EQ:
				r = (strcmp(r1, r2) == 0);
			break;
		}

		op = tokenizer_token(ctx);
	}

	return r;
}

const char* str_expr(struct ubasic_ctx* ctx)
{
	char* t1;
	char* t2;
	char tmp[1024];
	int op;

	t1 = (char*)str_factor(ctx);
	op = tokenizer_token(ctx);

	strlcpy(tmp, t1, 1024);
	
	while (op == PLUS)
	{
		tokenizer_next(ctx);
		t2 = (char*)str_factor(ctx);
		switch (op)
		{
			case PLUS:
				strlcat(tmp, t2, 1024);
			break;
		}
		op = tokenizer_token(ctx);
	}
	return gc_strdup(tmp);
}
