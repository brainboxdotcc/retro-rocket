#include <kernel.h>

void double_varfactor(struct basic_ctx* ctx, double* res)
{
	double r;

	basic_get_numeric_variable(tokenizer_variable_name(ctx), ctx, &r);

	// Special case for builin functions
	if (tokenizer_token(ctx) == COMMA) {
		tokenizer_error_print(ctx, "Too many parameters for builtin function");
	} else {
		if (tokenizer_token(ctx) == CLOSEBRACKET) {
			accept_or_return(CLOSEBRACKET, ctx);
		} else {
			accept_or_return(VARIABLE, ctx);
		}
	}
	*res = r;
}

void double_factor(struct basic_ctx* ctx, double* res)
{
	int tok = tokenizer_token(ctx);
	switch (tok) {
		case NUMBER:
			tokenizer_fnum(ctx, tok, res);
			accept_or_return(tok, ctx);
		break;
		case OPENBRACKET:
			accept_or_return(OPENBRACKET, ctx);
			double_expr(ctx, res);
			accept_or_return(CLOSEBRACKET, ctx);
		break;
		default:
			double_varfactor(ctx, res);
		break;
	}
}

void double_term(struct basic_ctx* ctx, double* res)
{
	double f1, f2;

	double_factor(ctx, &f1);

	int op = tokenizer_token(ctx);
	while (op == ASTERISK || op == SLASH || op == MOD) {
		tokenizer_next(ctx);
		double_factor(ctx, &f2);
		switch (op) {
			case ASTERISK:
				f1 = f1 * f2;
			break;
			case SLASH:
				if (f2 == 0.0) {
					tokenizer_error_print(ctx, "Division by zero");
					*res = 0.0;
				} else {
					f1 = f1 / f2;
				}
			break;
			case MOD:
				if (f2 == 0.0) {
					tokenizer_error_print(ctx, "Division by zero");
					*res = 0.0;
				} else {
					f1 = (int64_t)f1 % (int64_t)f2;
				}
			break;
		}
		op = tokenizer_token(ctx);
	}
	*res = f1;
}

void double_expr(struct basic_ctx* ctx, double* res)
{
	double t1, t2;

	double_term(ctx, &t1);
	int op = tokenizer_token(ctx);

	while (op == PLUS || op == MINUS || op == AND || op == OR) {
		tokenizer_next(ctx);
		double_term(ctx, &t2);
		switch (op) {
			case PLUS:
				t1 = t1 + t2;
			break;
			case MINUS:
				t1 = t1 - t2;
			break;
			case AND:
				t1 = (int64_t)t1 & (int64_t)t2;
			break;
			case OR:
				t1 = (int64_t)t1 | (int64_t)t2;
			break;
		}
		op = tokenizer_token(ctx);
	}
	*res = t1;
}

void double_relation(struct basic_ctx* ctx, double* res)
{
	double r1;
	double_expr(ctx, &r1);
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
		double r2;
		double_expr(ctx, &r2);

		switch (op) {
			case LESSTHAN:
				r1 = or_equal ? r1 <= r2  : r1 < r2;
			break;
			case GREATERTHAN:
				r1 = or_equal ? r1 >= r2 : r1 > r2;
			break;
			case EQUALS:
				r1 = not_equal ? r1 != r2 : r1 == r2;
			break;
		}

		op = tokenizer_token(ctx);
	}

	*res = r1;
}

