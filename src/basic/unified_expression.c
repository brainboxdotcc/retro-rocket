/**
 * @file basic/unified_expression.c
 * @brief Unified typed expression parser replacing the individual
 * ones for parsing int, double, string
 *
 * This file implements a single typed recursive-descent evaluator that
 * understands INT / REAL / STRING and relational + boolean operators.
 */

#include <kernel.h>
#include "basic/unified_expression.h"

/* ---------- Typed value ---------- */

/* Constructors */
up_value up_make_int(int64_t x) {
	up_value v;
	v.kind = UP_INT;
	v.v.i = x;
	return v;
}

up_value up_make_real(double x) {
	up_value v;
	v.kind = UP_REAL;
	v.v.r = x;
	return v;
}

up_value up_make_str(const char *s) {
	up_value v;
	v.kind = UP_STR;
	v.v.s = s;
	return v;
}

/* Promote INT -> REAL when needed */
static inline void up_promote_pair(up_value *a, up_value *b) {
	if (a->kind == UP_REAL || b->kind == UP_REAL) {
		if (a->kind == UP_INT) {
			a->v.r = (double) a->v.i;
			a->kind = UP_REAL;
		}
		if (b->kind == UP_INT) {
			b->v.r = (double) b->v.i;
			b->kind = UP_REAL;
		}
	}
}

/* Truthiness */
static inline int up_truth(const up_value *v) {
	switch (v->kind) {
		case UP_INT:
			return v->v.i != 0;
		case UP_REAL:
			return v->v.r != 0.0;
		case UP_STR:
			return v->v.s && v->v.s[0] != '\0';
	}
	return 0;
}

/* ---------- Forward decls ---------- */

static up_value up_value_expr(struct basic_ctx *ctx);   /* + / - and string + (above term) */
static up_value up_relation_expr(struct basic_ctx *ctx);/* typed relation: returns UP_INT(0/1) */
static up_value up_term(struct basic_ctx *ctx);

static up_value up_unary(struct basic_ctx *ctx);        /* { + | - }* factor */
static up_value up_factor(struct basic_ctx *ctx);

/* ---------- Factor ---------- */
/* factor := NUMBER | HEXNUMBER | STRING | VARIABLE | '(' expr ')' */
static up_value up_factor(struct basic_ctx *ctx) {
	enum token_t tok = tokenizer_token(ctx);

	switch (tok) {
		case NUMBER: {
			/* Distinguish 123 vs 1.23 by peeking a dot in the lexeme. */
			const char *p = ctx->ptr;
			while (isdigit(*p)) p++;
			if (*p == '.' && isdigit(p[1])) {
				double d = 0.0;
				tokenizer_fnum(ctx, tok, &d);
				accept(NUMBER, ctx);
				return up_make_real(d);
			} else {
				int64_t n = tokenizer_num(ctx, tok);
				accept(NUMBER, ctx);
				return up_make_int(n);
			}
		}
		case HEXNUMBER: {
			int64_t n = tokenizer_num(ctx, tok);
			accept(HEXNUMBER, ctx);
			return up_make_int(n);
		}
		case STRING: {
			if (!tokenizer_string(ctx->string, sizeof(ctx->string), ctx)) {
				tokenizer_error_print(ctx, "Bad string literal");
				return up_make_str("");
			}
			/* IMPORTANT: do NOT consume a ')' here; that belongs to the caller (e.g., func args). */
			accept(STRING, ctx);
			return up_make_str(gc_strdup(ctx, ctx->string));
		}
		case VARIABLE: {
			const char *name = tokenizer_variable_name(ctx);
			size_t L = name ? strlen(name) : 0;

			if (L && name[L - 1] == '$') {
				/* String var / string builtin (e.g. MID$) */
				const char *s = basic_get_string_variable(name, ctx);
				if (tokenizer_token(ctx) == CLOSEBRACKET) {
					accept(CLOSEBRACKET, ctx);
				} else {
					accept(VARIABLE, ctx);
				}
				return up_make_str(s ? s : "");
			}

			if (L && name[L - 1] == '#') {
				/* Explicit real-typed variable/builtin */
				double d = 0.0;
				basic_get_numeric_variable(name, ctx, &d);
				if (tokenizer_token(ctx) == COMMA) {
					tokenizer_error_print(ctx, "Too many parameters for builtin function");
				} else {
					if (tokenizer_token(ctx) == CLOSEBRACKET) {
						accept(CLOSEBRACKET, ctx);
					} else {
						accept(VARIABLE, ctx);
					}
				}
				return up_make_real(d);
			}

			if (is_builtin_double_fn(name)) {
				/* Unsuffixed builtin known to return REAL */
				double d = 0.0;
				basic_get_numeric_variable(name, ctx, &d);
				if (tokenizer_token(ctx) == COMMA) {
					tokenizer_error_print(ctx, "Too many parameters for builtin function");
				} else {
					if (tokenizer_token(ctx) == CLOSEBRACKET) {
						accept(CLOSEBRACKET, ctx);
					} else {
						accept(VARIABLE, ctx);
					}
				}
				return up_make_real(d);
			}

			/* Default: unsuffixed numeric treated as INT (keeps perf for plain vars) */
			int64_t n = basic_get_numeric_int_variable(name, ctx);
			if (tokenizer_token(ctx) == COMMA) {
				tokenizer_error_print(ctx, "Too many parameters for builtin function");
			} else {
				if (tokenizer_token(ctx) == CLOSEBRACKET) {
					accept(CLOSEBRACKET, ctx);
				} else {
					accept(VARIABLE, ctx);
				}
			}
			return up_make_int(n);
		}
		case OPENBRACKET: {
			accept(OPENBRACKET, ctx);
			up_value v = up_relation_expr(ctx);
			accept(CLOSEBRACKET, ctx);
			return v;
		}
		default:
			dprintf("Expected expression, current token: %d", tok);
			tokenizer_error_print(ctx, "Expected expression");
			/* Do not advance; return a benign zero to recover */
			return up_make_int(0);
	}
}

/* ---------- Unary ---------- */
/* unary := { PLUS | MINUS }* factor
 * Handles numeric negation/positivation; strings are not allowed.
 */
static up_value up_unary(struct basic_ctx *ctx) {
	int negate = 0;
	bool saw_sign = false;

	while (1) {
		enum token_t t = tokenizer_token(ctx);
		if (t == PLUS) {
			saw_sign = true;
			tokenizer_next(ctx);
			continue;
		}
		if (t == MINUS) {
			saw_sign = true;
			negate ^= 1;        /* flip on each '-' */
			tokenizer_next(ctx);
			continue;
		}
		break;
	}

	up_value v = up_factor(ctx);

	if (saw_sign) {
		/* Unary +/- applied - only valid for numeric. */
		if (v.kind == UP_STR) {
			tokenizer_error_print(ctx, "Unary +/- on string");
			/* recover with zero */
			return up_make_int(0);
		}
		if (negate) {
			if (v.kind == UP_REAL) {
				v.v.r = -v.v.r;
			} else {
				v.v.i = -v.v.i;
			}
		}
	}
	return v;
}

/* ---------- Term ---------- */
/* term := unary { (* | / | MOD) unary } */
static up_value up_term(struct basic_ctx *ctx) {
	up_value acc = up_unary(ctx);

	for (;;) {
		enum token_t op = tokenizer_token(ctx);
		if (op != ASTERISK && op != SLASH && op != MOD) {
			break;
		}

		tokenizer_next(ctx);
		up_value rhs = up_unary(ctx);

		/* Only numeric for * / MOD */
		if (acc.kind == UP_STR || rhs.kind == UP_STR) {
			tokenizer_error_print(ctx, "Numeric operator on string");
			/* recover by treating string as 0 */
			if (acc.kind == UP_STR) {
				acc = up_make_int(0);
			}
			if (rhs.kind == UP_STR) {
				rhs = up_make_int(0);
			}
		}

		up_promote_pair(&acc, &rhs);

		switch (op) {
			case ASTERISK:
				if (acc.kind == UP_REAL) {
					acc.v.r = acc.v.r * rhs.v.r;
				} else {
					acc.v.i = acc.v.i * rhs.v.i;
				}
				break;
			case SLASH:
				if (acc.kind == UP_REAL || rhs.kind == UP_REAL) {
					if (rhs.v.r == 0.0) {
						tokenizer_error_print(ctx, "Division by zero");
						acc = up_make_real(0.0);
					} else {
						/* ensure both real */
						if (acc.kind != UP_REAL) {
							acc = up_make_real((double) acc.v.i);
						}
						if (rhs.kind != UP_REAL) {
							rhs = up_make_real((double) rhs.v.i);
						}
						acc.v.r = acc.v.r / rhs.v.r;
					}
				} else {
					if (rhs.v.i == 0) {
						tokenizer_error_print(ctx, "Division by zero");
						acc = up_make_int(0);
					} else {
						acc.v.i = acc.v.i / rhs.v.i;
					}
				}
				break;
			case MOD:
				if (acc.kind == UP_REAL || rhs.kind == UP_REAL) {
					/* coerce to ints */
					int64_t a = (acc.kind == UP_REAL) ? (int64_t) acc.v.r : acc.v.i;
					int64_t b = (rhs.kind == UP_REAL) ? (int64_t) rhs.v.r : rhs.v.i;
					if (b == 0) {
						tokenizer_error_print(ctx, "Division by zero");
						acc = up_make_int(0);
					} else {
						acc = up_make_int(a % b);
					}
				} else {
					if (rhs.v.i == 0) {
						tokenizer_error_print(ctx, "Division by zero");
						acc = up_make_int(0);
					} else {
						acc.v.i = acc.v.i % rhs.v.i;
					}
				}
				break;
			default:
		}
	}

	return acc;
}

/* ---------- Additive / concat ---------- */
/* value_expr := term { (+ | -) term }
 * '+' does numeric add for numeric operands, and string concatenation for strings.
 * Mixed string/numeric is an error (mirrors current split evaluators).
 */
static up_value up_value_expr(struct basic_ctx *ctx) {
	up_value acc = up_term(ctx);

	for (;;) {
		enum token_t op = tokenizer_token(ctx);
		if (op != PLUS && op != MINUS) {
			break;
		}

		tokenizer_next(ctx);
		up_value rhs = up_term(ctx);

		if (op == PLUS && acc.kind == UP_STR && rhs.kind == UP_STR) {
			/* String concatenation */
			char tmp[MAX_STRINGLEN];
			tmp[0] = '\0';
			if (acc.v.s) {
				strlcpy(tmp, acc.v.s, sizeof(tmp));
			}
			if (rhs.v.s) {
				strlcat(tmp, rhs.v.s, sizeof(tmp));
			}
			acc = up_make_str(gc_strdup(ctx, tmp));
			continue;
		}

		/* Numeric add/sub only; forbid mixing string with number */
		if (acc.kind == UP_STR || rhs.kind == UP_STR) {
			tokenizer_error_print(ctx, "Cannot mix string and number with '+' or '-'");
			/* recover: treat strings as 0 */
			if (acc.kind == UP_STR) {
				acc = up_make_int(0);
			}
			if (rhs.kind == UP_STR) {
				rhs = up_make_int(0);
			}
		}

		up_promote_pair(&acc, &rhs);

		if (op == PLUS) {
			if (acc.kind == UP_REAL) {
				acc.v.r = acc.v.r + rhs.v.r;
			} else {
				acc.v.i = acc.v.i + rhs.v.i;
			}
		} else { /* MINUS */
			if (acc.kind == UP_REAL) {
				acc.v.r = acc.v.r - rhs.v.r;
			} else {
				acc.v.i = acc.v.i - rhs.v.i;
			}
		}
	}

	return acc;
}

/* ---------- Relational ---------- */
/* relation_expr := value_expr { (< | > | =) [= or ><] value_expr }
 * Produces UP_INT(0/1).
 */
static up_value up_relation_expr(struct basic_ctx *ctx) {
	up_value lhs = up_value_expr(ctx);
	enum token_t op = tokenizer_token(ctx);

	while (op == LESSTHAN || op == GREATERTHAN || op == EQUALS) {
		tokenizer_next(ctx);

		enum token_t secondary = tokenizer_token(ctx);
		int mode = 0; /* 0: plain, 1: or_equal, 2: not_equal */
		if (op == LESSTHAN || op == GREATERTHAN) {
			if (secondary == EQUALS) {
				mode = 1; /* <= or >= */
				tokenizer_next(ctx);
			} else if (op == LESSTHAN && secondary == GREATERTHAN) {
				/* <> */
				op = EQUALS;
				mode = 2; /* not equal */
				tokenizer_next(ctx);
			}
		}

		up_value rhs = up_value_expr(ctx);

		int result = 0;
		if (lhs.kind == UP_STR || rhs.kind == UP_STR) {
			/* Both must be strings to compare. */
			if (lhs.kind != UP_STR || rhs.kind != UP_STR) {
				tokenizer_error_print(ctx, "Cannot compare string with number");
				result = 0;
			} else {
				int cmp = strcmp(lhs.v.s ? lhs.v.s : "", rhs.v.s ? rhs.v.s : "");
				if (op == LESSTHAN) {
					result = (mode == 1) ? (cmp <= 0) : (cmp < 0);
				} else if (op == GREATERTHAN) {
					result = (mode == 1) ? (cmp >= 0) : (cmp > 0);
				} else {
					/* EQUALS */
					result = (mode == 2) ? (cmp != 0) : (cmp == 0);
				}
			}
		} else {
			/* Numeric comparison (promote as needed). */
			up_promote_pair(&lhs, &rhs);
			if (lhs.kind == UP_REAL) {
				if (op == LESSTHAN) {
					result = (mode == 1) ? (lhs.v.r <= rhs.v.r) : (lhs.v.r < rhs.v.r);
				} else if (op == GREATERTHAN) {
					result = (mode == 1) ? (lhs.v.r >= rhs.v.r) : (lhs.v.r > rhs.v.r);
				} else {
					/* EQUALS */
					result = (mode == 2) ? (lhs.v.r != rhs.v.r) : (lhs.v.r == rhs.v.r);
				}
			} else {
				if (op == LESSTHAN) {
					result = (mode == 1) ? (lhs.v.i <= rhs.v.i) : (lhs.v.i < rhs.v.i);
				} else if (op == GREATERTHAN) {
					result = (mode == 1) ? (lhs.v.i >= rhs.v.i) : (lhs.v.i > rhs.v.i);
				} else {
					/* EQUALS */
					result = (mode == 2) ? (lhs.v.i != rhs.v.i) : (lhs.v.i == rhs.v.i);
				}
			}
		}

		lhs = up_make_int(result);
		op = tokenizer_token(ctx);
	}

	return lhs; /* UP_INT(0/1) */
}

/* NOT binds tighter than AND/OR */
static up_value parse_bool_term(struct basic_ctx *ctx) {
	/* consume one-or-more logical NOTs */
	int negate = 0;
	while (tokenizer_token(ctx) == NOT) {
		tokenizer_next(ctx);
		negate ^= 1;
	}

	/* a boolean term is a typed relation that yields UP_INT(0/1) */
	up_value b = up_relation_expr(ctx);
	if (b.kind != UP_INT) {
		b = up_make_int(up_truth(&b)); /* normalise just in case */
	}
	if (negate) {
		b.v.i = !b.v.i;
	}
	return b;
}

/* ---------- Boolean conditional ---------- */
/* bool_term := [NOT]* relation_expr
 * conditional := bool_term { (AND | OR) bool_term }
 */
bool up_conditional(struct basic_ctx *ctx) {
	/* Parse one NOT*-prefixed boolean term */
	up_value acc = parse_bool_term(ctx);

	for (;;) {
		enum token_t tk = tokenizer_token(ctx);
		if (tk == AND) {
			tokenizer_next(ctx);
			up_value rhs = parse_bool_term(ctx);
			acc.v.i = (up_truth(&acc) && up_truth(&rhs));
			acc.kind = UP_INT;
			continue;
		}
		if (tk == OR) {
			tokenizer_next(ctx);
			up_value rhs = parse_bool_term(ctx);
			acc.v.i = (up_truth(&acc) || up_truth(&rhs));
			acc.kind = UP_INT;
			continue;
		}
		break; /* caller can expect THEN/NEWLINE/EOF here */
	}

	return up_truth(&acc);
}

/* ---------- Optional strict shims (use when rolling in) ---------- */
/* These let you start swapping call-sites safely, one by one. */

int64_t up_int_expr_strict(struct basic_ctx *ctx)
/* Numeric expression; errors if it evaluates to a string. */
{
	up_value v = up_value_expr(ctx);
	if (v.kind == UP_STR) {
		tokenizer_error_print(ctx, "String in numeric expression");
		return 0;
	}
	if (v.kind == UP_REAL) {
		/* Mirror current behaviour: narrow like double_expr does for bitwise ops. */
		return (int64_t) v.v.r;
	}
	return v.v.i;
}

void up_double_expr_strict(struct basic_ctx *ctx, double *out)
/* Real expression; errors if it evaluates to a string. */
{
	up_value v = up_value_expr(ctx);
	if (v.kind == UP_STR) {
		tokenizer_error_print(ctx, "String in numeric expression");
		*out = 0.0;
		return;
	}
	if (v.kind == UP_REAL) {
		*out = v.v.r;
		return;
	}
	*out = (double) v.v.i;
}

const char *up_str_expr_strict(struct basic_ctx *ctx)
/* String expression; errors if it evaluates to a number. */
{
	up_value v = up_value_expr(ctx);
	if (v.kind != UP_STR) {
		tokenizer_error_print(ctx, "Numeric value in string expression");
		return "";
	}
	return v.v.s ? v.v.s : "";
}

/* For completeness, a typed relation that returns 0/1 (int) */
int64_t up_relation_i(struct basic_ctx *ctx) {
	up_value b = up_relation_expr(ctx);
	return (b.kind == UP_INT) ? b.v.i : up_truth(&b);
}

void up_eval_value(struct basic_ctx *ctx, up_value *out) {
	if (!out) {
		return;
	}
	*out = up_value_expr(ctx);
}
