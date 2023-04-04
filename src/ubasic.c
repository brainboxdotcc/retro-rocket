/*
 * BBC BASIC interpreter,
 * Retro Rocket OS Project (C) Craig Edwards 2012.
 * loosely based on uBASIC (Copyright (c) 2006, Adam Dunkels, All rights reserved).
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#define DEBUG 1

#if DEBUG
#define DEBUG_PRINTF(...)  dprintf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#define ubasic_error(str) tokenizer_error_print(ctx, str)

#include <kernel.h>

static int64_t expr(struct ubasic_ctx* ctx);
static void double_expr(struct ubasic_ctx* ctx, double* res);
static void line_statement(struct ubasic_ctx* ctx);
static void statement(struct ubasic_ctx* ctx);
static const char* str_expr(struct ubasic_ctx* ctx);
const char* str_varfactor(struct ubasic_ctx* ctx);
void ubasic_parse_fn(struct ubasic_ctx* ctx);
int64_t ubasic_getproccount(struct ubasic_ctx* ctx);
int64_t ubasic_getprocid(struct ubasic_ctx* ctx);
char* ubasic_getprocname(struct ubasic_ctx* ctx);
char* ubasic_dns(struct ubasic_ctx* ctx);
int64_t ubasic_rgb(struct ubasic_ctx* ctx);
void ubasic_eval_double_fn(const char* fn_name, struct ubasic_ctx* ctx, double* res);

struct ubasic_int_fn builtin_int[] =
{
	{ ubasic_abs, "ABS" },
	{ ubasic_len, "LEN" },
	{ ubasic_openin, "OPENIN" },
	{ ubasic_eof, "EOF" },
	{ ubasic_read, "READ" },
	{ ubasic_instr, "INSTR" },
	{ ubasic_asc, "ASC" },
	{ ubasic_getnamecount, "GETNAMECOUNT" },
	{ ubasic_getsize, "GETSIZE" },
	{ ubasic_getproccount, "GETPROCCOUNT" },
	{ ubasic_getprocid, "GETPROCID" },
	{ ubasic_rgb, "RGB" },
	{ NULL, NULL }
};

struct ubasic_double_fn builtin_double[] = {
	{ NULL, NULL },
};

struct ubasic_str_fn builtin_str[] =
{
	{ ubasic_netinfo, "NETINFO$" },
	{ ubasic_dns, "DNS$" },
	{ ubasic_left, "LEFT$" },
	{ ubasic_mid, "MID$" },
	{ ubasic_chr, "CHR$" },
	{ ubasic_readstring, "READ$" },
	{ ubasic_getname, "GETNAME$" },
	{ ubasic_getprocname, "GETPROCNAME$" },
	{ NULL, NULL }
};

char* clean_basic(const char* program, char* output_buffer)
{
	bool in_quotes = false;
	uint16_t bracket_depth = 0;
	const char* p = program;
	char *d = output_buffer;
	while (*p) {
		while (*p == '\r') {
			p++;
		}
		if (*p == '"') {
			in_quotes = !in_quotes;
		} else if (*p == '(' && !in_quotes) {
			bracket_depth++;
		} else if (*p == ')' && !in_quotes) {
			bracket_depth--;
		}
		if (!in_quotes && bracket_depth > 0) {
			if (*p != ' ') {
				*d++ = *p++;
			} else {
				p++;
			}
		} else {
			*d++ = *p++;
		}
		// Remove extra newlines
		if (*(p - 1) == '\n' || *(p - 1) == '\r') {
			while (*p == '\r' || *p == '\n') {
				p++;
			}
		}
	}
	// Terminate output
	*d = 0;
	return output_buffer;
}

void set_system_variables(struct ubasic_ctx* ctx, uint32_t pid)
{
	ubasic_set_int_variable("TRUE", 1, ctx, false, false);
	ubasic_set_int_variable("FALSE", 0, ctx, false, false);
	ubasic_set_int_variable("PID", pid, ctx, false, false);
	ubasic_set_double_variable("PI#", 3.141592653589793238, ctx, false, false);
}

/*---------------------------------------------------------------------------*/
struct ubasic_ctx* ubasic_init(const char *program, console* cons, uint32_t pid, const char* file)
{
	struct ubasic_ctx* ctx = kmalloc(sizeof(struct ubasic_ctx));
	int i;

	ctx->current_token = TOKENIZER_ERROR;	
	ctx->int_variables = NULL;
	ctx->str_variables = NULL;
	ctx->double_variables = NULL;
	for (i = 0; i < MAX_GOSUB_STACK_DEPTH; i++)
		ctx->local_int_variables[i] = NULL;
	for (i = 0; i < MAX_GOSUB_STACK_DEPTH; i++)
		ctx->local_string_variables[i] = NULL;
	for (i = 0; i < MAX_GOSUB_STACK_DEPTH; i++)
		ctx->local_double_variables[i] = NULL;
	ctx->cons = (struct console*)cons;
	ctx->oldlen = 0;
	ctx->fn_return = NULL;
	// We allocate 5000 bytes extra on the end of the program for EVAL space,
	// as EVAL appends to the program on lines 9998 and 9999.
	ctx->program_ptr = (char*)kmalloc(strlen(program) + 5000);

	// Clean extra whitespace from the program
	ctx->program_ptr = clean_basic(program, ctx->program_ptr);

	ctx->for_stack_ptr = ctx->gosub_stack_ptr = 0;
	ctx->defs = NULL;

	ctx->graphics_colour = 0xFFFFFF;

	// Scan the program for functions and procedures

        tokenizer_init(ctx->program_ptr, ctx);
	ubasic_parse_fn(ctx);

	set_system_variables(ctx, pid);
	ubasic_set_string_variable("PROGRAM$", file, ctx, false, false);

	return ctx;
}

struct ubasic_ctx* ubasic_clone(struct ubasic_ctx* old)
{
	struct ubasic_ctx* ctx = (struct ubasic_ctx*)kmalloc(sizeof(struct ubasic_ctx));
	int i;

	ctx->current_token = TOKENIZER_ERROR;
	ctx->int_variables = old->int_variables;
	ctx->str_variables = old->str_variables;
	ctx->double_variables = old->double_variables;

	for (i = 0; i < MAX_GOSUB_STACK_DEPTH; i++)
		ctx->local_int_variables[i] = old->local_int_variables[i];
	for (i = 0; i < MAX_GOSUB_STACK_DEPTH; i++)
		ctx->local_string_variables[i] = old->local_string_variables[i];
	for (i = 0; i < MAX_GOSUB_STACK_DEPTH; i++)
		ctx->local_double_variables[i] = old->local_double_variables[i];

	ctx->cons = old->cons;
	ctx->oldlen = old->oldlen;
	ctx->fn_return = NULL;
	ctx->program_ptr = old->program_ptr;
	ctx->for_stack_ptr = old->for_stack_ptr;
	ctx->gosub_stack_ptr = old->gosub_stack_ptr;
	ctx->defs = old->defs;

	tokenizer_init(ctx->program_ptr, ctx);

	return ctx;
}

void ubasic_parse_fn(struct ubasic_ctx* ctx)
{
	int currentline = 0;

	while (1)
	{
		currentline = tokenizer_num(ctx, TOKENIZER_NUMBER);
		char const* linestart = ctx->ptr;
		do
		{
			do
			{
				tokenizer_next(ctx);
			}
			while (tokenizer_token(ctx) != TOKENIZER_CR && tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT);
			
			char const* lineend = ctx->ptr;
			
			char* linetext = (char*)kmalloc(lineend - linestart + 1);
			strlcpy(linetext, linestart, lineend - linestart + 1);

			char* search = linetext;

			while (*search++ >= '0' && *search <= '9')
				search++;
			--search;

			while (*search++ == ' ');
			--search;

			if (!strncmp(search, "DEF ", 4))
			{
				search += 4;
				ub_fn_type type = FT_FN;
				if (!strncmp(search, "FN", 2))
				{
					search += 2;
					while (*search++ == ' ');
					type = FT_FN;
				}
				else if (!strncmp(search, "PROC", 4))
				{
					search += 4;
					while (*search++ == ' ');
					type = FT_PROC;
				}

				char name[1024];
				int ni = 0;
				struct ub_proc_fn_def* def = (struct ub_proc_fn_def*)kmalloc(sizeof(struct ub_proc_fn_def));
				--search;
				while (ni < 1023 && *search != '\n' && *search != 0 && *search != '(')
				{
					name[ni++] = *search++;
				}
				name[ni] = 0;

				def->name = strdup(name);
				def->type = type;
				def->line = currentline;
				def->next = ctx->defs;

				/* Parse parameters */

				def->params = NULL;

				if (*search == '(')
				{
					search++;
					// Parse parameters
					char pname[1024];
					int pni = 0;
					while (*search != 0)
					{
						if (pni < 1023 && *search != ',' && *search != ')' && *search != ' ')
							pname[pni++] = *search;

						if (*search == ',' || *search == ')')
						{
							pname[pni] = 0;
							struct ub_param* par = (struct ub_param*)kmalloc(sizeof(struct ub_param));
							//kprintf("pn='%s'\n", pname);

							par->next = NULL;
							par->name = strdup(pname);

							if (def->params == NULL)
							{
								def->params = par;
							}
							else
							{
								struct ub_param* cur = def->params;
								for (; cur; cur = cur->next)
								{
									if (cur->next == NULL)
									{
										cur->next = par;
										break;
									}
								}
							}

							if (*search == ')')
								break;

							pni = 0;
						}

						search++;

					}
				}

				ctx->defs = def;

				//kprintf("Name='%s'\n", name);
			}


			if (tokenizer_token(ctx) == TOKENIZER_CR)
			{
				tokenizer_next(ctx);
			}

			kfree(linetext);

			if (tokenizer_token(ctx) == TOKENIZER_ENDOFINPUT) {
				break;
			}
		}
		while (tokenizer_token(ctx) != TOKENIZER_NUMBER && tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT);

		if (tokenizer_token(ctx) == TOKENIZER_ENDOFINPUT) {
			break;
		}
	}

	tokenizer_init(ctx->program_ptr, ctx);

	ctx->ended = 0;
	return;
}

struct ub_proc_fn_def* ubasic_find_fn(const char* name, struct ubasic_ctx* ctx)
{
	struct ub_proc_fn_def* cur = ctx->defs;
	for (; cur; cur = cur->next)
	{
		if (!strcmp(name, cur->name))
			return cur;
	}
	return NULL;
}

void ubasic_destroy(struct ubasic_ctx* ctx)
{
	for (; ctx->int_variables; ctx->int_variables = ctx->int_variables->next)
	{
		kfree(ctx->int_variables->varname);
		kfree(ctx->int_variables);
	}
	for (; ctx->str_variables; ctx->str_variables = ctx->str_variables->next)
	{
		kfree(ctx->str_variables->varname);
		kfree(ctx->str_variables->value);
		kfree(ctx->str_variables);
	}
	kfree((char*)ctx->program_ptr);
	kfree(ctx);
}

/*---------------------------------------------------------------------------*/
static void accept(int token, struct ubasic_ctx* ctx)
{
	char err[MAX_STRINGLEN];
	sprintf(err, "Expected %s got %s", types[token], types[tokenizer_token(ctx)]);
	if (token != tokenizer_token(ctx)) {
		tokenizer_error_print(ctx, err);
		return;
	}
	tokenizer_next(ctx);
}
/*---------------------------------------------------------------------------*/
static int64_t varfactor(struct ubasic_ctx* ctx)
{
	int64_t r = ubasic_get_int_variable(tokenizer_variable_name(ctx), ctx);
	// Special case for builin functions
	if (tokenizer_token(ctx) == TOKENIZER_COMMA)
		tokenizer_error_print(ctx, "Too many parameters for builtin function");
	else
	{
		if (tokenizer_token(ctx) == TOKENIZER_RIGHTPAREN)
			accept(TOKENIZER_RIGHTPAREN, ctx);
		else
			accept(TOKENIZER_VARIABLE, ctx);
	}
	return r;
}

static void double_varfactor(struct ubasic_ctx* ctx, double* res)
{
	double r;

	ubasic_get_double_variable(tokenizer_variable_name(ctx), ctx, &r);

	dprintf("double_varfactor\n");

	// Special case for builin functions
	if (tokenizer_token(ctx) == TOKENIZER_COMMA)
		tokenizer_error_print(ctx, "Too many parameters for builtin function");
	else
	{
		if (tokenizer_token(ctx) == TOKENIZER_RIGHTPAREN)
			accept(TOKENIZER_RIGHTPAREN, ctx);
		else
			accept(TOKENIZER_VARIABLE, ctx);
	}
	*res = r;
}

const char* str_varfactor(struct ubasic_ctx* ctx)
{
	const char* r;
	if (*ctx->ptr == '"')
	{
		if (!tokenizer_string(ctx->string, sizeof(ctx->string), ctx)) {
			return "";
		}
		if (tokenizer_token(ctx) == TOKENIZER_RIGHTPAREN)
			accept(TOKENIZER_RIGHTPAREN, ctx);
		else
			accept(TOKENIZER_STRING, ctx);
		return ctx->string;
	}
	else
	{
		r = ubasic_get_string_variable(tokenizer_variable_name(ctx), ctx);
		if (tokenizer_token(ctx) == TOKENIZER_RIGHTPAREN)
			accept(TOKENIZER_RIGHTPAREN, ctx);
		else
			accept(TOKENIZER_VARIABLE, ctx);
	}
	return r;
}

/*---------------------------------------------------------------------------*/
static int64_t factor(struct ubasic_ctx* ctx)
{
	int64_t r = 0;

	int tok = tokenizer_token(ctx);
	switch (tok) {
		case TOKENIZER_NUMBER:
		case TOKENIZER_HEXNUMBER:
			r = tokenizer_num(ctx, tok);
			accept(tok, ctx);
		break;
		case TOKENIZER_LEFTPAREN:
			accept(TOKENIZER_LEFTPAREN, ctx);
			r = expr(ctx);
			accept(TOKENIZER_RIGHTPAREN, ctx);
		break;
		default:
			r = varfactor(ctx);
		break;
	}
	return r;
}

static void double_factor(struct ubasic_ctx* ctx, double* res)
{
	char buffer[50];

	int tok = tokenizer_token(ctx);
	switch (tok) {
		case TOKENIZER_NUMBER:
			dprintf("double_factor TOKENIZER_NUMBER\n");
			tokenizer_fnum(ctx, tok, res);
			dprintf("double_factor fnum->r=%s\n", double_to_string(*res, buffer, 50, 0));
			accept(tok, ctx);
		break;
		case TOKENIZER_LEFTPAREN:
			dprintf("double_factor TOKENIZER_LEFTPAREN\n");
			accept(TOKENIZER_LEFTPAREN, ctx);
			double_expr(ctx, res);
			dprintf("double_factor expr->r=%s\n", double_to_string(*res, buffer, 50, 0));
			accept(TOKENIZER_RIGHTPAREN, ctx);
		break;
		default:
			dprintf("double_factor default\n");
			double_varfactor(ctx, res);
			dprintf("double_factor varfactor->r=%s\n", double_to_string(*res, buffer, 50, 0));
		break;
	}
	dprintf("double_factor end, r=%s\n", double_to_string(*res, buffer, 50, 0));
}

static const char* str_factor(struct ubasic_ctx* ctx)
{
	const char* r;

	switch(tokenizer_token(ctx))
	{
		case TOKENIZER_LEFTPAREN:
			accept(TOKENIZER_LEFTPAREN, ctx);
			r = str_expr(ctx);
			accept(TOKENIZER_RIGHTPAREN, ctx);
		break;
		default:
			r = str_varfactor(ctx);
		break;
	}

	return r;
}

/*---------------------------------------------------------------------------*/
static int64_t term(struct ubasic_ctx* ctx)
{
	int64_t f1, f2;
	int op;

	f1 = factor(ctx);
	//kprintf("Factor is %d\n", f1);
	op = tokenizer_token(ctx);
	while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH || op == TOKENIZER_MOD)
	{
		tokenizer_next(ctx);
		f2 = factor(ctx);
		switch (op)
		{
			case TOKENIZER_ASTR:
				f1 = f1 * f2;
			break;
			case TOKENIZER_SLASH:
				f1 = f1 / f2;
			break;
			case TOKENIZER_MOD:
				f1 = f1 % f2;
			break;
		}
		op = tokenizer_token(ctx);
	}
	return f1;
}

static void double_term(struct ubasic_ctx* ctx, double* res)
{
	double f1, f2;
	int op;
	char buffer[50];

	dprintf("double_term first double_factor call\n");
	double_factor(ctx, &f1);
	dprintf("double_term f1=%s\n", double_to_string(f1, buffer, 50, 0));

	op = tokenizer_token(ctx);
	dprintf("first op=%d %s\n", op, types[op]);
	while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH || op == TOKENIZER_MOD)
	{
		tokenizer_next(ctx);
		dprintf("double_term second double_factor call\n");
		double_factor(ctx, &f2);
		dprintf("double_term f2=%s\n", double_to_string(f2, buffer, 50, 0));
		switch (op)
		{
			case TOKENIZER_ASTR:
				f1 = f1 * f2;
			break;
			case TOKENIZER_SLASH:
				f1 = f1 / f2;
			break;
			case TOKENIZER_MOD:
				f1 = (int64_t)f1 % (int64_t)f2;
			break;
		}
		op = tokenizer_token(ctx);
	}
	dprintf("final op=%d %s\n", op, types[op]);
	dprintf("double_term returning %s\n", double_to_string(f1, buffer, 50, 0));
	*res = f1;
}

/*---------------------------------------------------------------------------*/
static int64_t expr(struct ubasic_ctx* ctx)
{
	int64_t t1, t2;
	int op;

	t1 = term(ctx);
	op = tokenizer_token(ctx);

	while (op == TOKENIZER_PLUS || op == TOKENIZER_MINUS || op == TOKENIZER_AND || op == TOKENIZER_OR)
	{
		tokenizer_next(ctx);
		t2 = term(ctx);
		switch (op)
		{
			case TOKENIZER_PLUS:
				t1 = t1 + t2;
			break;
			case TOKENIZER_MINUS:
				t1 = t1 - t2;
			break;
			case TOKENIZER_AND:
				t1 = t1 & t2;
			break;
			case TOKENIZER_OR:
				t1 = t1 | t2;
			break;
		}
		op = tokenizer_token(ctx);
	}
	return t1;
}

static void double_expr(struct ubasic_ctx* ctx, double* res)
{
	double t1, t2;
	int op;

	dprintf("double_expr()\n");

	double_term(ctx, &t1);
	op = tokenizer_token(ctx);
	dprintf("double_expr before type, type is %d %s\n", op, types[op]);

	while (op == TOKENIZER_PLUS || op == TOKENIZER_MINUS || op == TOKENIZER_AND || op == TOKENIZER_OR) {
		dprintf("double_expr after type, type is %d %s\n", op, types[op]);
		tokenizer_next(ctx);
		dprintf("double_expr call 2nd double_term\n");
		double_term(ctx, &t2);
		switch (op) {
			case TOKENIZER_PLUS:
				dprintf("tokenizer plus\n");
				t1 = t1 + t2;
			break;
			case TOKENIZER_MINUS:
				dprintf("tokenizer minus\n");
				t1 = t1 - t2;
			break;
			case TOKENIZER_AND:
				dprintf("tokenizer and\n");
				t1 = (int64_t)t1 & (int64_t)t2;
			break;
			case TOKENIZER_OR:
				dprintf("tokenizer or\n");
				t1 = (int64_t)t1 | (int64_t)t2;
			break;
		}
		op = tokenizer_token(ctx);
	}
	dprintf("double_expr done\n");
	*res = t1;
}


static const char* str_expr(struct ubasic_ctx* ctx)
{
	char* t1;
	char* t2;
	char tmp[1024];
	int op;

	t1 = (char*)str_factor(ctx);
	op = tokenizer_token(ctx);

	strlcpy(tmp, t1, 1024);
	
	while (op == TOKENIZER_PLUS)
	{
		tokenizer_next(ctx);
		t2 = (char*)str_factor(ctx);
		switch (op)
		{
			case TOKENIZER_PLUS:
				strlcat(tmp, t2, 1024);
			break;
		}
		op = tokenizer_token(ctx);
	}
	return gc_strdup(tmp);
}
/*---------------------------------------------------------------------------*/
static int relation(struct ubasic_ctx* ctx)
{
	int r1, r2;
	int op;

	r1 = expr(ctx);
	op = tokenizer_token(ctx);

	while (op == TOKENIZER_LT || op == TOKENIZER_GT || op == TOKENIZER_EQ)
	{
		tokenizer_next(ctx);
		r2 = expr(ctx);

		switch (op)
		{
			case TOKENIZER_LT:
				r1 = r1 < r2;
			break;
			case TOKENIZER_GT:
				r1 = r1 > r2;
			break;
			case TOKENIZER_EQ:
				r1 = r1 == r2;
			break;
		}

		op = tokenizer_token(ctx);
	}

	return r1;
}

static int str_relation(struct ubasic_ctx* ctx)
{
	int op, r;

	const char* r1 = str_expr(ctx);
	op = tokenizer_token(ctx);

	while (op == TOKENIZER_LT || op == TOKENIZER_GT || op == TOKENIZER_EQ)
	{
		tokenizer_next(ctx);
		const char* r2 = str_expr(ctx);

		switch (op)
		{
			case TOKENIZER_LT:
				r = (strcmp(r1, r2) < 0);
			break;
			case TOKENIZER_GT:
				r = (strcmp(r1, r2) > 0);
			break;
			case TOKENIZER_EQ:
				r = (strcmp(r1, r2) == 0);
			break;
		}

		op = tokenizer_token(ctx);
	}

	return r;
}


/*---------------------------------------------------------------------------*/
void jump_linenum(int64_t linenum, struct ubasic_ctx* ctx)
{
	tokenizer_init(ctx->program_ptr, ctx);

	while (tokenizer_num(ctx, TOKENIZER_NUMBER) != linenum && !tokenizer_finished(ctx)) {
		if (tokenizer_num(ctx, TOKENIZER_NUMBER) > linenum) {
			/* If the line we just found is greater than the one we are searching for, we can bail early */
			tokenizer_error_print(ctx, "No such line");
			return;
		}
		do {
			do {
				tokenizer_next(ctx);
			}
			while (tokenizer_token(ctx) != TOKENIZER_CR && !tokenizer_finished(ctx));

			if (tokenizer_token(ctx) == TOKENIZER_CR) {
				tokenizer_next(ctx);
			}
		}
		while(tokenizer_token(ctx) != TOKENIZER_NUMBER && !tokenizer_finished(ctx));

		if (tokenizer_finished(ctx)) {
			tokenizer_error_print(ctx, "No such line");
			return;
		}
	}
}
/*---------------------------------------------------------------------------*/
static void goto_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_GOTO, ctx);
	jump_linenum(tokenizer_num(ctx, TOKENIZER_NUMBER), ctx);
}

static void colour_statement(struct ubasic_ctx* ctx, int tok)
{
	accept(tok, ctx);
	setforeground((console*)ctx->cons, expr(ctx));
	accept(TOKENIZER_CR, ctx);
}

static char* printable_syntax(struct ubasic_ctx* ctx)
{
	int numprints = 0;
	int no_newline = 0;
	int next_hex = 0;
	char buffer[MAX_STRINGLEN], out[MAX_STRINGLEN];
	
	*out = 0;

	do {
		no_newline = 0;
		*buffer = 0;
		if (tokenizer_token(ctx) == TOKENIZER_STRING) {
			if (tokenizer_string(ctx->string, sizeof(ctx->string), ctx)) {
				sprintf(buffer, "%s", ctx->string);
				strlcat(out, buffer, MAX_STRINGLEN);
			} else {
				tokenizer_error_print(ctx, "Unterminated \"");
				return NULL;
			}
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == TOKENIZER_COMMA) {
			strlcat(out, "\t", MAX_STRINGLEN);
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == TOKENIZER_SEMICOLON) {
			no_newline = 1;
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == TOKENIZER_TILDE) {
			next_hex = 1;
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == TOKENIZER_PLUS) {
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == TOKENIZER_VARIABLE || tokenizer_token(ctx) == TOKENIZER_NUMBER || tokenizer_token(ctx) == TOKENIZER_HEXNUMBER) {
			/* Check if it's a string or numeric expression */
			const char* oldctx = ctx->ptr;
			if (tokenizer_token(ctx) != TOKENIZER_NUMBER && tokenizer_token(ctx) != TOKENIZER_HEXNUMBER && (*ctx->ptr == '"' || strchr(tokenizer_variable_name(ctx), '$'))) {
				ctx->ptr = oldctx;
				sprintf(buffer, "%s", str_expr(ctx));
				strlcat(out, buffer, MAX_STRINGLEN);
			} else {
				ctx->ptr = oldctx;
				const char* var_name = tokenizer_variable_name(ctx);
				ctx->ptr = oldctx;
				bool printable_double = (strchr(var_name, '#') || tokenizer_decimal_number(ctx));
				if (printable_double) {
					double f = 0.0;
					char double_buffer[32];
					ctx->ptr = oldctx;
					double_expr(ctx, &f);
					//sprintf(buffer, "%f", f);
					strlcat(buffer, double_to_string(f, double_buffer, 32, 0), MAX_STRINGLEN);
				} else {
					ctx->ptr = oldctx;
					sprintf(buffer, next_hex ? "%lX" : "%ld", expr(ctx));
				}
				strlcat(out, buffer, MAX_STRINGLEN);
				next_hex = 0;
			}
		} else {
			break;
		}
		numprints++;
	}
  	while(tokenizer_token(ctx) != TOKENIZER_CR && tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT && numprints < 255);
  
	if (!no_newline) {
		strlcat(out, "\n", MAX_STRINGLEN);
	}

	tokenizer_next(ctx);
	return gc_strdup(out);
}

/*---------------------------------------------------------------------------*/
static void print_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_PRINT, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		kprintf(out);
	}
}

static void sockwrite_statement(struct ubasic_ctx* ctx)
{
	int fd = -1;

	accept(TOKENIZER_SOCKWRITE, ctx);
	fd = ubasic_get_int_variable(tokenizer_variable_name(ctx), ctx);
	accept(TOKENIZER_VARIABLE, ctx);
	accept(TOKENIZER_COMMA, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		send(fd, out, strlen(out));
	}
}

/*---------------------------------------------------------------------------*/
static void if_statement(struct ubasic_ctx* ctx)
{
	int r;
  
	accept(TOKENIZER_IF, ctx);

	char current_line[MAX_STRINGLEN];
	char* pos = strchr(ctx->ptr, '\n');
	char* end = strchr(ctx->ptr, 0);
	bool stringlike = false;
	strlcpy(current_line, ctx->ptr, pos ? pos - ctx->ptr + 1 : end - ctx->ptr + 1);
	if (strlen(current_line) > 10) { // "IF 1 THEN ..."
		for (char* n = current_line; *n; ++n) {
			if (*n == '$') {
				stringlike = true;
				break;
			} else if (*n == ' ' && *(n+1) == 'T' && *(n+2) == 'H' && *(n+3) == 'E' && *(n+4) == 'N') {
				break;
			}
		}
	}

	r = stringlike ? str_relation(ctx) : relation(ctx);
	accept(TOKENIZER_THEN, ctx);
	if (r)
	{
		statement(ctx);
	}
	else
	{
		do
		{
			tokenizer_next(ctx);
		}
   		while(tokenizer_token(ctx) != TOKENIZER_ELSE &&
				tokenizer_token(ctx) != TOKENIZER_CR &&
				tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT);

		if (tokenizer_token(ctx) == TOKENIZER_ELSE)
		{
			tokenizer_next(ctx);
			statement(ctx);
		}
   			else if (tokenizer_token(ctx) == TOKENIZER_CR)
		{
			tokenizer_next(ctx);
		}
	}
}

static void chain_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_CHAIN, ctx);
	const char* pn = str_expr(ctx);
	//kprintf("Chaining '%s'\n", pn);
	struct process* p = proc_load(pn, ctx->cons);
	struct ubasic_ctx* new_proc = p->code;
	struct ub_var_int* cur_int = ctx->int_variables;
	struct ub_var_string* cur_str = ctx->str_variables;
	struct ub_var_double* cur_double = ctx->double_variables;

	for (; cur_int; cur_int = cur_int->next) {
		if (cur_int->global) {
			ubasic_set_int_variable(cur_int->varname, cur_int->value, new_proc, false, true);
		}
	}
	for (; cur_str; cur_str = cur_str->next) {
		if (cur_str->global) {
			ubasic_set_string_variable(cur_str->varname, cur_str->value, new_proc, false, true);
		}
	}
	for (; cur_double; cur_double = cur_double->next) {
		if (cur_double->global) {
			ubasic_set_double_variable(cur_double->varname, cur_double->value, new_proc, false, true);
		}
	}

	/* Inherit global variables into new process */
	proc_wait(proc_cur(), p->pid);
	accept(TOKENIZER_CR, ctx);
}

static void eval_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_EVAL, ctx);
	const char* v = str_expr(ctx);
	accept(TOKENIZER_CR, ctx);

	if (ctx->current_linenum == 9998) {
		ctx->eval_linenum = 0;
		tokenizer_error_print(ctx, "Recursive EVAL");
		return;
	}

	char clean_v[MAX_STRINGLEN];
	clean_basic(v, clean_v);

	if (ctx->oldlen == 0) {
		ubasic_set_string_variable("ERROR$", "", ctx, false, false);
		ubasic_set_int_variable("ERROR", 0, ctx, false, false);
		ctx->oldlen = strlen(ctx->program_ptr);
		strlcat(ctx->program_ptr, "9998 ", ctx->oldlen + 5000);
		strlcat(ctx->program_ptr, clean_v, ctx->oldlen + 5000);
		strlcat(ctx->program_ptr, "\n9999 RETURN\n", ctx->oldlen + 5000);

		ctx->eval_linenum = ctx->current_linenum;

		ctx->gosub_stack[ctx->gosub_stack_ptr++] = ctx->current_linenum;

		jump_linenum(9998, ctx);
	} else {
		ctx->program_ptr[ctx->oldlen] = 0;
		ctx->oldlen = 0;
		ctx->eval_linenum = 0;
	}
}

static void rem_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_REM, ctx);
	while (tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT && tokenizer_token(ctx) != TOKENIZER_CR)
		tokenizer_next(ctx);
	accept(TOKENIZER_CR, ctx);
}

static void def_statement(struct ubasic_ctx* ctx)
{
	// Because the function or procedure definition is pre-parsed by ubasic_init(),
	// we just skip the entire line moving to the next if we hit a DEF statement.
	// in the future we should check if the interpreter is actually calling a FN,
	// to check we dont fall through into a function.
	accept(TOKENIZER_DEF, ctx);
	while (tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT && tokenizer_token(ctx) != TOKENIZER_CR)
		tokenizer_next(ctx);
	accept(TOKENIZER_CR, ctx);
}

static void openin_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "OPENIN is a function");
}

static void read_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "READ is a function");
}

static void close_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_CLOSE, ctx);
	_close(expr(ctx));
	accept(TOKENIZER_CR, ctx);
}

static void eof_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "EOF is a function");
}

static void input_statement(struct ubasic_ctx* ctx)
{
	const char* var;

	accept(TOKENIZER_INPUT, ctx);
	var = tokenizer_variable_name(ctx);
	accept(TOKENIZER_VARIABLE, ctx);

	if (kinput(10240, (console*)ctx->cons) != 0)
	{
		switch (var[strlen(var) - 1])
		{
			case '$':
				ubasic_set_string_variable(var, kgetinput((console*)ctx->cons), ctx, false, false);
			break;
			case '#':
				double f = 0;
				atof(kgetinput((console*)ctx->cons), &f);
				ubasic_set_double_variable(var, f, ctx, false, false);
			break;
			default:
				ubasic_set_int_variable(var, atoll(kgetinput((console*)ctx->cons), 10), ctx, false, false);
			break;
		}

		kfreeinput((console*)ctx->cons);

		accept(TOKENIZER_CR, ctx);
	}
	else
	{
		jump_linenum(ctx->current_linenum, ctx);
	}
}

static void sockread_statement(struct ubasic_ctx* ctx)
{
	char input[MAX_STRINGLEN];
	const char* var = NULL;
	int fd = -1;

	dprintf("S");

	accept(TOKENIZER_SOCKREAD, ctx);
	fd = ubasic_get_int_variable(tokenizer_variable_name(ctx), ctx);
	accept(TOKENIZER_VARIABLE, ctx);
	accept(TOKENIZER_COMMA, ctx);
	var = tokenizer_variable_name(ctx);
	accept(TOKENIZER_VARIABLE, ctx);

	int rv = recv(fd, input, MAX_STRINGLEN, false, 10);

	if (rv > 0) {
		*(input + rv) = 0;
		switch (var[strlen(var) - 1]) {
			case '$':
				ubasic_set_string_variable(var, input, ctx, false, false);
			break;
			case '#':
				double f = 0;
				atof(input, &f);
				ubasic_set_double_variable(var, f, ctx, false, false);
			break;
			default:
				ubasic_set_int_variable(var, atoll(input, 10), ctx, false, false);
			break;
		}

		accept(TOKENIZER_CR, ctx);
	} else if (rv < 0) {
		tokenizer_error_print(ctx, socket_error(rv));
	} else {
		jump_linenum(ctx->current_linenum, ctx);
	}
}

static void connect_statement(struct ubasic_ctx* ctx)
{
	char input[MAX_STRINGLEN];
	const char* fd_var = NULL, *ip = NULL;
	int64_t port = 0;

	accept(TOKENIZER_CONNECT, ctx);
	fd_var = tokenizer_variable_name(ctx);
	accept(TOKENIZER_VARIABLE, ctx);
	accept(TOKENIZER_COMMA, ctx);
	ip = str_expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	port = expr(ctx);

	int rv = connect(str_to_ip(ip), port, 0, true);

	if (rv >= 0) {
		*(input + rv) = 0;
		switch (fd_var[strlen(fd_var) - 1]) {
			case '$':
				tokenizer_error_print(ctx, "Can't store socket descriptor in STRING");
			break;
			case '#':
				tokenizer_error_print(ctx, "Cannot store socket descriptor in REAL");
			break;
			default:
				ubasic_set_int_variable(fd_var, rv, ctx, false, false);
			break;
		}

		accept(TOKENIZER_CR, ctx);
	} else {
		tokenizer_error_print(ctx, socket_error(rv));
	}
}

static void sockclose_statement(struct ubasic_ctx* ctx)
{
	const char* fd_var = NULL;

	accept(TOKENIZER_SOCKCLOSE, ctx);
	fd_var = tokenizer_variable_name(ctx);
	accept(TOKENIZER_VARIABLE, ctx);

	int rv = closesocket(ubasic_get_int_variable(fd_var, ctx));
	if (rv == 0) {
		// Clear variable to -1
		ubasic_set_int_variable(fd_var, -1, ctx, false, false);
		accept(TOKENIZER_CR, ctx);
	} else {
		tokenizer_error_print(ctx, socket_error(rv));
	}
}

/*---------------------------------------------------------------------------*/
static void let_statement(struct ubasic_ctx* ctx, bool global)
{
	const char* var;
	const char* _expr;
	double f_expr = 0;

	var = tokenizer_variable_name(ctx);
	accept(TOKENIZER_VARIABLE, ctx);
	accept(TOKENIZER_EQ, ctx);

	dprintf("let '%s' = ...\n", var);

	switch (var[strlen(var) - 1])
	{
		case '$':
			_expr = str_expr(ctx);
			ubasic_set_string_variable(var, _expr, ctx, false, global);
		break;
		case '#':
			dprintf("double: ctx before call: %s\n", ctx->ptr);
			double_expr(ctx, &f_expr);
			ubasic_set_double_variable(var, f_expr, ctx, false, global);
		break;
		default:
			dprintf("int: ctx before call: %s\n", ctx->ptr);
			ubasic_set_int_variable(var, expr(ctx), ctx, false, global);
		break;
	}
	accept(TOKENIZER_CR, ctx);
}

static void cls_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_CLS, ctx);
	clearscreen(current_console);
	accept(TOKENIZER_CR, ctx);
}

static void gcol_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_GCOL, ctx);
	ctx->graphics_colour = expr(ctx);
	dprintf("New graphics color: %08X\n", ctx->graphics_colour);
	accept(TOKENIZER_CR, ctx);
}

static void draw_line_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_LINE, ctx);
	int64_t x1 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept(TOKENIZER_CR, ctx);
	draw_line(x1, y1, x2, y2, ctx->graphics_colour);
}

static void triangle_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_TRIANGLE, ctx);
	int64_t x1 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t x3 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t y3 = expr(ctx);
	accept(TOKENIZER_CR, ctx);
	draw_triangle(x1, y1, x2, y2, x3, y3, ctx->graphics_colour);
}

static void rectangle_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_RECTANGLE, ctx);
	int64_t x1 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept(TOKENIZER_CR, ctx);
	draw_horizontal_rectangle(x1, y1, x2, y2, ctx->graphics_colour);
}

static void circle_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_CIRCLE, ctx);
	int64_t x = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t y = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t radius = expr(ctx);
	accept(TOKENIZER_COMMA, ctx);
	int64_t filled = expr(ctx);
	accept(TOKENIZER_CR, ctx);
	draw_circle(x, y, radius, filled, ctx->graphics_colour);
}

/*---------------------------------------------------------------------------*/
static void gosub_statement(struct ubasic_ctx* ctx)
{
	int linenum;

	accept(TOKENIZER_GOSUB, ctx);
	linenum = tokenizer_num(ctx, TOKENIZER_NUMBER);
	accept(TOKENIZER_NUMBER, ctx);
	accept(TOKENIZER_CR, ctx);

	if (ctx->gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH)
	{
		ctx->gosub_stack[ctx->gosub_stack_ptr] = tokenizer_num(ctx, TOKENIZER_NUMBER);
		ctx->gosub_stack_ptr++;
		jump_linenum(linenum, ctx);
	}
	else
	{
		tokenizer_error_print(ctx, "gosub_statement: gosub stack exhausted");
	}
}
/*---------------------------------------------------------------------------*/
static void return_statement(struct ubasic_ctx* ctx)
{
	//kprintf("Return\n");
	accept(TOKENIZER_RETURN, ctx);
	if (ctx->gosub_stack_ptr > 0) {
		ctx->gosub_stack_ptr--;
		jump_linenum(ctx->gosub_stack[ctx->gosub_stack_ptr], ctx);
	} else {
		tokenizer_error_print(ctx, "return_statement: non-matching return");
	}
}
/*---------------------------------------------------------------------------*/
static void next_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_NEXT, ctx);
	if (ctx->for_stack_ptr > 0) {
		int incr = ubasic_get_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ctx);
		//kprintf("incr is %d\n", incr);
		ubasic_set_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ++incr, ctx, false, false);

		if (incr < ctx->for_stack[ctx->for_stack_ptr - 1].to) {
			jump_linenum(ctx->for_stack[ctx->for_stack_ptr - 1].line_after_for, ctx);
		} else {
			kfree(ctx->for_stack[ctx->for_stack_ptr].for_variable);
			ctx->for_stack_ptr--;
			accept(TOKENIZER_CR, ctx);
		}
	} else {
		tokenizer_error_print(ctx, "next_statement: non-matching next");
		accept(TOKENIZER_CR, ctx);
	}
}

/*---------------------------------------------------------------------------*/
static void for_statement(struct ubasic_ctx* ctx)
{
	const char* for_variable;
	int to;
  
	accept(TOKENIZER_FOR, ctx);
	for_variable = strdup(tokenizer_variable_name(ctx));
	accept(TOKENIZER_VARIABLE, ctx);
	accept(TOKENIZER_EQ, ctx);
	ubasic_set_int_variable(for_variable, expr(ctx), ctx, false, false);
	accept(TOKENIZER_TO, ctx);
	to = expr(ctx);
	accept(TOKENIZER_CR, ctx);

	if (ctx->for_stack_ptr < MAX_FOR_STACK_DEPTH) {
		ctx->for_stack[ctx->for_stack_ptr].line_after_for = tokenizer_num(ctx, TOKENIZER_NUMBER);
		ctx->for_stack[ctx->for_stack_ptr].for_variable = (char*)for_variable;
		ctx->for_stack[ctx->for_stack_ptr].to = to;
		DEBUG_PRINTF("for_statement: new for, var %s to %d\n", ctx->for_stack[ctx->for_stack_ptr].for_variable, ctx->for_stack[ctx->for_stack_ptr].to); 
		ctx->for_stack_ptr++;
	} else {
		tokenizer_error_print(ctx, "Too many FOR");
	}
}

/*---------------------------------------------------------------------------*/
static void end_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_END, ctx);
	ctx->ended = 1;
}

static void eq_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_EQ, ctx);

	if (ctx->fn_type == RT_STRING) {
		ctx->fn_return = (void*)str_expr(ctx);
	} else {
		ctx->fn_return = (void*)expr(ctx);
	}

	accept(TOKENIZER_CR, ctx);

	ctx->ended = 1;
}

/*---------------------------------------------------------------------------*/
static void statement(struct ubasic_ctx* ctx)
{
	int token = tokenizer_token(ctx);

	switch (token) {
		case TOKENIZER_REM:
			rem_statement(ctx);
		break;
		case TOKENIZER_COLOR:
			colour_statement(ctx, TOKENIZER_COLOR);
		break;
		case TOKENIZER_COLOUR:
			colour_statement(ctx, TOKENIZER_COLOUR);
		break;
		case TOKENIZER_DEF:
			def_statement(ctx);
		break;
		case TOKENIZER_CHAIN:
			chain_statement(ctx);
		break;
		case TOKENIZER_EVAL:
			eval_statement(ctx);
		break;
		case TOKENIZER_OPENIN:
			openin_statement(ctx);
		break;
		case TOKENIZER_READ:
			read_statement(ctx);
		break;
		case TOKENIZER_CLOSE:
			close_statement(ctx);
		break;
		case TOKENIZER_EOF:
			eof_statement(ctx);
		break;
		case TOKENIZER_PRINT:
			print_statement(ctx);
		break;
		case TOKENIZER_IF:
			if_statement(ctx);
		break;
		case TOKENIZER_GOTO:
			goto_statement(ctx);
		break;
		case TOKENIZER_GOSUB:
			gosub_statement(ctx);
		break;
		case TOKENIZER_RETURN:
			return_statement(ctx);
		break;
		case TOKENIZER_FOR:
			for_statement(ctx);
		break;
		case TOKENIZER_NEXT:
			next_statement(ctx);
		break;
		case TOKENIZER_END:
			end_statement(ctx);
		break;
		case TOKENIZER_INPUT:
			input_statement(ctx);
		break;
		case TOKENIZER_SOCKREAD:
			sockread_statement(ctx);
		break;
		case TOKENIZER_SOCKWRITE:
			sockwrite_statement(ctx);
		break;
		case TOKENIZER_CONNECT:
			connect_statement(ctx);
		break;
		case TOKENIZER_SOCKCLOSE:
			sockclose_statement(ctx);
		break;
		case TOKENIZER_CLS:
			cls_statement(ctx);
		break;
		case TOKENIZER_GCOL:
			gcol_statement(ctx);
		break;
		case TOKENIZER_LINE:
			draw_line_statement(ctx);
		break;
		case TOKENIZER_TRIANGLE:
			triangle_statement(ctx);
		break;
		case TOKENIZER_RECTANGLE:
			rectangle_statement(ctx);
		break;
		case TOKENIZER_CIRCLE:
			circle_statement(ctx);
		break;
		case TOKENIZER_LET:
			accept(TOKENIZER_LET, ctx);
			/* Fall through. */
		case TOKENIZER_VARIABLE:
			let_statement(ctx, false);
		break;
		case TOKENIZER_GLOBAL:
			accept(TOKENIZER_GLOBAL, ctx);
			let_statement(ctx, true);
		break;
		case TOKENIZER_EQ:
			eq_statement(ctx);
		break;
		default:
			tokenizer_error_print(ctx, "Unknown keyword");
			//kprintf("%s\n ->%d\n", ctx->program_ptr, ctx->current_token);
		break;
	}
}
/*---------------------------------------------------------------------------*/
static void line_statement(struct ubasic_ctx* ctx)
{
	ctx->current_linenum = tokenizer_num(ctx, TOKENIZER_NUMBER);
	accept(TOKENIZER_NUMBER, ctx);
	statement(ctx);
	return;
}
/*---------------------------------------------------------------------------*/
void ubasic_run(struct ubasic_ctx* ctx)
{
	if (ubasic_finished(ctx)) {
		return;
	}
	line_statement(ctx);
	gc();
}
/*---------------------------------------------------------------------------*/
int ubasic_finished(struct ubasic_ctx* ctx)
{
	int st = ctx->ended || tokenizer_finished(ctx);
	return st;
}
/*---------------------------------------------------------------------------*/
void ubasic_set_variable(const char* var, const char* value, struct ubasic_ctx* ctx)
{
	/* first, itdentify the variable's type. Look for $ for string,
	 * and parenthesis for arrays
	 */
	const char last_letter = var[strlen(var) - 1];

	switch (last_letter) {
		case '$':
			ubasic_set_string_variable(var, value, ctx, 0, false);
		break;
		case '#':
			double f = 0.0;
			atof(value, &f);
			ubasic_set_double_variable(var, f, ctx, 0, false);
		break;
		case ')':
			ubasic_set_array_variable(var, atoll(value, 10), ctx, 0);
		break;
		default:
			ubasic_set_int_variable(var, atoll(value, 10), ctx, 0, false);
		break;
	}
}

int valid_string_var(const char* name)
{
	const char* i;
	unsigned int varLength = strlen(name);
	if (varLength < 2 || name[varLength - 1] != '$') {
		return 0;
	}
	for (i = name; *i != '$'; i++) {
		if (*i == '$' && *(i + 1) != 0) {
		       return 0;
		}
		if ((*i < 'A' || *i > 'Z') && (*i < 'a' || *i > 'z')) {
			return 0;
		}
	}
	return 1;
}

int valid_double_var(const char* name)
{
	const char* i;
	unsigned int varLength = strlen(name);
	if (varLength < 2 || name[varLength - 1] != '#') {
		return 0;
	}
	for (i = name; *i != '#'; i++) {
		if (*i == '#' && *(i + 1) != 0) {
		       return 0;
		}
		if ((*i < 'A' || *i > 'Z') && (*i < 'a' || *i > 'z')) {
			return 0;
		}
	}
	return 1;
}

int valid_int_var(const char* name)
{
	const char* i;
	unsigned int varLength = strlen(name);
	if (varLength < 1) {
		return 0;
	}
	for (i = name; *i; i++) {
		if ((*i < 'A' || *i > 'Z') && (*i < 'a' || *i > 'z')) {
			return 0;
		}
	}
	return 1;
}

void ubasic_set_string_variable(const char* var, const char* value, struct ubasic_ctx* ctx, bool local, bool global)
{
	struct ub_var_string* list[] = {
		ctx->str_variables,
		ctx->local_string_variables[ctx->gosub_stack_ptr]
	};

	//kprintf("set string '%s' to '%s' %d\n", var, value, local);

	if (!valid_string_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}
	if (list[local] == NULL) {
		if (local) {
			ctx->local_string_variables[ctx->gosub_stack_ptr] = (struct ub_var_string*)kmalloc(sizeof(struct ub_var_string));
			ctx->local_string_variables[ctx->gosub_stack_ptr]->next = NULL;
			ctx->local_string_variables[ctx->gosub_stack_ptr]->varname = strdup(var);
			ctx->local_string_variables[ctx->gosub_stack_ptr]->value = strdup(value);
			ctx->local_string_variables[ctx->gosub_stack_ptr]->global = global;
		} else {
			ctx->str_variables = (struct ub_var_string*)kmalloc(sizeof(struct ub_var_string));
			ctx->str_variables->next = NULL;
			ctx->str_variables->varname = strdup(var);
			ctx->str_variables->value = strdup(value);
			ctx->str_variables->global = global;
		}
		return;
	} else {
		struct ub_var_string* cur = ctx->str_variables;
		if (local) {
			cur = ctx->local_string_variables[ctx->gosub_stack_ptr];
		}
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname))	{
				kfree(cur->value);
				cur->value = strdup(value);
				cur->global = global;
				return;
			}
		}
		struct ub_var_string* newvar = (struct ub_var_string*)kmalloc(sizeof(struct ub_var_string));
		if (local) {
			newvar->next = ctx->local_string_variables[ctx->gosub_stack_ptr];
		} else {
			newvar->next = ctx->str_variables;
		}

		newvar->next = ctx->str_variables;
		newvar->varname = strdup(var);
		newvar->value = strdup(value);
		newvar->global = global;
	
		if (local) {
			ctx->local_string_variables[ctx->gosub_stack_ptr] = newvar;
		} else {
			ctx->str_variables = newvar;
		}
	}
}

void ubasic_set_array_variable(const char* var, int64_t value, struct ubasic_ctx* ctx, bool local)
{
}

void ubasic_set_int_variable(const char* var, int64_t value, struct ubasic_ctx* ctx, bool local, bool global)
{
	struct ub_var_int* list[] = {
		ctx->int_variables,
		ctx->local_int_variables[ctx->gosub_stack_ptr]
	};

	if (!valid_int_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}

	if (list[local] == NULL) {
		if (local) {
			dprintf("Set int variable '%s' to '%d' (gosub local)\n", var, value);
			ctx->local_int_variables[ctx->gosub_stack_ptr] = kmalloc(sizeof(struct ub_var_int));
			ctx->local_int_variables[ctx->gosub_stack_ptr]->next = NULL;
			ctx->local_int_variables[ctx->gosub_stack_ptr]->varname = strdup(var);
			ctx->local_int_variables[ctx->gosub_stack_ptr]->value = value;
		} else {
			dprintf("Set int variable '%s' to '%d' (default)\n", var, value);
			ctx->int_variables = kmalloc(sizeof(struct ub_var_int));
			ctx->int_variables->next = NULL;
			ctx->int_variables->varname = strdup(var);
			ctx->int_variables->value = value;
		}
		return;
	} else {
		struct ub_var_int* cur = ctx->int_variables;
		if (local)
			cur = ctx->local_int_variables[ctx->gosub_stack_ptr];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname)) {
				dprintf("Set int variable '%s' to '%d' (updating)\n", var, value);
				cur->value = value;
				return;
			}
		}
		dprintf("Set int variable '%s' to '%d'\n", var, value);
		struct ub_var_int* newvar = kmalloc(sizeof(struct ub_var_int));
		newvar->next = (local ? ctx->local_int_variables[ctx->gosub_stack_ptr] : ctx->int_variables);
		newvar->varname = strdup(var);
		newvar->value = value;
		if (local) {
			ctx->local_int_variables[ctx->gosub_stack_ptr] = newvar;
		} else {
			ctx->int_variables = newvar;
		}
	}
}

void ubasic_set_double_variable(const char* var, double value, struct ubasic_ctx* ctx, bool local, bool global)
{
	struct ub_var_double* list[] = {
		ctx->double_variables,
		ctx->local_double_variables[ctx->gosub_stack_ptr]
	};
	char buffer[MAX_STRINGLEN];

	if (!valid_double_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}

	if (list[local] == NULL) {
		if (local) {
			dprintf("Set double variable '%s' to '%s' (gosub local)\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
			ctx->local_double_variables[ctx->gosub_stack_ptr] = kmalloc(sizeof(struct ub_var_double));
			ctx->local_double_variables[ctx->gosub_stack_ptr]->next = NULL;
			ctx->local_double_variables[ctx->gosub_stack_ptr]->varname = strdup(var);
			ctx->local_double_variables[ctx->gosub_stack_ptr]->value = value;
		} else {
			dprintf("Set double variable '%s' to '%s' (default)\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
			ctx->double_variables = kmalloc(sizeof(struct ub_var_double));
			ctx->double_variables->next = NULL;
			ctx->double_variables->varname = strdup(var);
			ctx->double_variables->value = value;
		}
		return;
	} else {
		struct ub_var_double* cur = ctx->double_variables;
		if (local)
			cur = ctx->local_double_variables[ctx->gosub_stack_ptr];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname)) {
				dprintf("Set double variable '%s' to '%s' (updating)\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
				cur->value = value;
				return;
			}
		}
		dprintf("Set double variable '%s' to '%s'\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
		struct ub_var_double* newvar = kmalloc(sizeof(struct ub_var_double));
		newvar->next = (local ? ctx->local_double_variables[ctx->gosub_stack_ptr] : ctx->double_variables);
		newvar->varname = strdup(var);
		newvar->value = value;
		if (local) {
			ctx->local_double_variables[ctx->gosub_stack_ptr] = newvar;
		} else {
			ctx->double_variables = newvar;
		}
	}
}

void begin_comma_list(struct ub_proc_fn_def* def, struct ubasic_ctx* ctx) {
	ctx->bracket_depth = 0;
	ctx->param = def->params;
	ctx->item_begin = (char*)ctx->ptr;
}


uint8_t extract_comma_list(struct ub_proc_fn_def* def, struct ubasic_ctx* ctx) {
	if (*ctx->ptr == '(') {
		ctx->bracket_depth++;
		if (ctx->bracket_depth == 1) {
			ctx->item_begin = ctx->ptr + 1;
		}
	}
	else if (*ctx->ptr == ')') {
		ctx->bracket_depth--;
	}
	if ((*ctx->ptr == ',' && ctx->bracket_depth == 1) || (*ctx->ptr == ')' && ctx->bracket_depth == 0)) {
		// next item
		// set local vars here
		// Set ctx to item_begin, call expr(), set ctx back again. Change expr to stop on comma.
		//
		// XXX We know wether to call expr or str_expr here based upon the type for the fn param
		// which we will read when this is implemented. We should probably check the fn exists
		// before we even GET here!
		char oldval = *ctx->ptr;
		char oldct = ctx->current_token;
		char* oldptr = (char*)ctx->ptr;
		char* oldnextptr = (char*)ctx->nextptr;
		ctx->nextptr = ctx->item_begin;
		ctx->ptr = ctx->item_begin;
		ctx->current_token = get_next_token(ctx);
		*oldptr = 0;
		if (ctx->param) {
			size_t len = strlen(ctx->param->name);
			if (ctx->param->name[len - 1] == '$') {
				ubasic_set_string_variable(ctx->param->name, str_expr(ctx), ctx, true, false);
			} else if (ctx->param->name[len - 1] == '#') {
				double f = 0.0;
				double_expr(ctx, &f);
				ubasic_set_double_variable(ctx->param->name, f, ctx, true, false);
			} else {
				ubasic_set_int_variable(ctx->param->name, expr(ctx), ctx, true, false);
			}

			ctx->param = ctx->param->next;
		}
		*oldptr = oldval;
		ctx->ptr = oldptr;
		ctx->nextptr = oldnextptr;
		ctx->current_token = oldct;
		ctx->item_begin = (char*)ctx->ptr + 1;
	}

	ctx->ptr++;

	if (ctx->bracket_depth == 0 || *ctx->ptr == 0) {
		ctx->nextptr = ctx->ptr;
		return 0;
	}

	return 1;

}

const char* ubasic_eval_str_fn(const char* fn_name, struct ubasic_ctx* ctx)
{
	struct ub_proc_fn_def* def = ubasic_find_fn(fn_name + 2, ctx);
	const char* rv = gc_strdup("");
	if (def != NULL) {
		ctx->gosub_stack_ptr++;
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct ubasic_ctx* atomic = ubasic_clone(ctx);
		atomic->fn_type = RT_STRING;
		jump_linenum(def->line, atomic);

		while (!ubasic_finished(atomic)) {
			line_statement(atomic);
		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
			rv = (const char*)atomic->fn_return;
		}

		/* Only free the base struct! */
		kfree(atomic);

		ctx->gosub_stack_ptr--;

		return rv;
	}
	tokenizer_error_print(ctx, "No such string FN");
	return rv;
}

#define BIP_STRING 0
#define BIP_INT 1

#define PARAMS_START \
	[[maybe_unused]] int itemtype = BIP_INT; \
	[[maybe_unused]] int64_t intval = 0; \
	[[maybe_unused]] char* strval = NULL; \
	[[maybe_unused]] char oldval = 0; \
	[[maybe_unused]] char oldct = 0; \
	[[maybe_unused]] char* oldptr = 0; \
	[[maybe_unused]] char const* oldnextptr = NULL; \
	[[maybe_unused]] int gotone = 0; \
	[[maybe_unused]] int bracket_depth = 0; \
	[[maybe_unused]] char const* item_begin = ctx->ptr;

#define PARAMS_GET_ITEM(type) { gotone = 0; \
	while (!gotone) \
	{ \
		if (*ctx->ptr == '(') { \
			bracket_depth++; \
			if (bracket_depth == 1) \
				item_begin = ctx->ptr + 1; \
		} \
       		else if (*ctx->ptr == ')') \
			bracket_depth--; \
		if ((*ctx->ptr == ',' && bracket_depth == 1) || (*ctx->ptr == ')' && bracket_depth == 0)) \
		{ \
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
			if (itemtype == BIP_STRING) \
			{ \
				strval = (char*)str_expr(ctx); \
			} \
			else \
			{ \
				intval = expr(ctx); \
			} \
			*oldptr = oldval; \
			ctx->ptr = oldptr; \
			ctx->nextptr = oldnextptr; \
			ctx->current_token = oldct; \
			item_begin = ctx->ptr + 1; \
			gotone = 1; \
		} \
		if (bracket_depth == 0 || *ctx->ptr == 0) \
		{ \
			ctx->nextptr = ctx->ptr; \
			ctx->current_token = get_next_token(ctx); \
			gotone = 1; \
		} \
		ctx->ptr++; \
	} \
}

#define PARAMS_END(NAME) { \
	ctx->ptr--; \
	if (*ctx->ptr != ')') \
		tokenizer_error_print(ctx, "Too many parameters for function " NAME); \
}

int64_t ubasic_openin(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	int fd = _open(strval, _O_RDONLY);
	return fd;
}

int64_t ubasic_getnamecount(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	fs_directory_entry_t* fsl = fs_get_items(strval);
	int count = 0;
	while (fsl)
	{
		fsl = fsl->next;
		count++;
	}
	return count;
}

int64_t ubasic_getproccount(struct ubasic_ctx* ctx)
{
	return proc_total();
}

int64_t ubasic_getprocid(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	return proc_id(intval);
}

int64_t ubasic_rgb(struct ubasic_ctx* ctx)
{
	int64_t r, g, b;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	r = intval;
	PARAMS_GET_ITEM(BIP_INT);
	g = intval;
	PARAMS_GET_ITEM(BIP_INT);
	b = intval;
	return (uint32_t)(r << 16 | g << 8 | b);
}

char* ubasic_getprocname(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	return gc_strdup(proc_name(intval));
}

char* ubasic_getname(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	fs_directory_entry_t* fsl = fs_get_items(strval);
	int count = 0;
	while (fsl)
	{
		if (count++ == intval)
		{
			return gc_strdup(fsl->filename);
		}
		fsl = fsl->next;
	}
	return gc_strdup("");
}

int64_t ubasic_getsize(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	fs_directory_entry_t* fsl = fs_get_items(strval);
	int count = 0;
	while (fsl)
	{
		if (count++ == intval)
		{
			return fsl->size;
		}
		fsl = fsl->next;
	}
	return 0;
}

int64_t ubasic_eof(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	return _eof(intval);
}

int64_t ubasic_asc(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	return *strval;
}

char* ubasic_chr(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	char res[2] = {(unsigned char)intval, 0};
	return gc_strdup(res);
}

int64_t ubasic_instr(struct ubasic_ctx* ctx)
{
	uint32_t i;
	char* haystack;
	char* needle;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	haystack = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	needle = strval;
	for (i = 0; i < strlen(haystack) - strlen(needle) + 1; ++i)
		if (!strncmp(haystack + i, needle, strlen(needle)))
			return i + 1;
	return 0;
}

char* ubasic_readstring(struct ubasic_ctx* ctx)
{
	//kprintf("read string\n");
	char* res = (char*)kmalloc(1024);
	int ofs = 0;
	*res = 0;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	while (!_eof(intval) && ofs < 1024)
	{
		if (_read(intval, res + ofs, 1) != 1)
			tokenizer_error_print(ctx, "Error reading from file");
		if (*(res + ofs) == '\n')
			break;
		else
			ofs++;
		//kprintf("Got byte %d", res[ofs - 1]);
	}
	*(res+ofs) = 0;
	char* ret = gc_strdup(res);
	kfree(res);
	return ret;
}

int64_t ubasic_read(struct ubasic_ctx* ctx)
{
	char res;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	if (_read(intval, &res, 1) != 1)
		tokenizer_error_print(ctx, "Error reading from file");
	return res;
}

char* ubasic_netinfo(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char ip[16] = { 0 };
	if (!stricmp(strval, "ip")) {
		unsigned char raw[4];
		if (gethostaddr(raw)) {
			get_ip_str(ip, (uint8_t*)&raw);
			return gc_strdup(ip);
		}
		return gc_strdup("0.0.0.0");
	}
	if (!stricmp(strval, "gw")) {
		uint32_t raw = getgatewayaddr();
		get_ip_str(ip, (uint8_t*)&raw);
		return gc_strdup(ip);
	}
	if (!stricmp(strval, "mask")) {
		uint32_t raw = getnetmask();
		get_ip_str(ip, (uint8_t*)&raw);
		return gc_strdup(ip);
	}
	if (!stricmp(strval, "dns")) {
		uint32_t raw = getdnsaddr();
		get_ip_str(ip, (uint8_t*)&raw);
		return gc_strdup(ip);
	}
	return gc_strdup("0.0.0.0");
}

char* ubasic_dns(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char ip[16] = { 0 };
	uint32_t addr = dns_lookup_host(getdnsaddr(), strval, 2);
	get_ip_str(ip, (uint8_t*)&addr);
	return gc_strdup(ip);
}

char* ubasic_left(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	if (intval > strlen(strval) || intval < 0)
		intval = strlen(strval);
	char* cut = gc_strdup(strval);
	*(cut + intval) = 0;
	return cut;
}

char* ubasic_mid(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	int64_t start = intval;
	intval = 0;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t end = intval;
	if (start > strlen(strval) || start < 0) {
		start = 0;
	}
	if (end > strlen(strval) || end < start) {
		end = start;
	}
	char* cut = gc_strdup(strval);
	*(cut + end) = 0;
	return cut + start;
}

int64_t ubasic_len(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	return strlen(strval);
}

int64_t ubasic_abs(struct ubasic_ctx* ctx)
{
	PARAMS_START;

	PARAMS_GET_ITEM(BIP_INT);
	int v = intval;

	if (v < 0)
		return 0 + -v;
	else
		return v;
}

/**
 * @brief Check if a function name is a builtin function returning an integer value,
 * if it is, call its handler and set its return value.
 * 
 * @param fn_name function name to look for
 * @param ctx interpreter context
 * @param res pointer to return value of function
 * @return uint64_t true/false return
 */
char ubasic_builtin_int_fn(const char* fn_name, struct ubasic_ctx* ctx, int64_t* res) {
	int i;
	for (i = 0; builtin_int[i].name; ++i) {
		if (!strcmp(fn_name, builtin_int[i].name)) {
			*res = builtin_int[i].handler(ctx);
			return 1;
		}
	}
	return 0;
}

char ubasic_builtin_double_fn(const char* fn_name, struct ubasic_ctx* ctx, double* res) {
	int i;
	for (i = 0; builtin_double[i].name; ++i) {
		if (!strcmp(fn_name, builtin_double[i].name)) {
			builtin_double[i].handler(ctx, res);
			return 1;
		}
	}
	return 0;
}

/**
 * @brief Check if a function name is a builtin function returning a string,
 * if it is, call its handler and set its return value.
 * 
 * @param fn_name function name to look for
 * @param ctx interpreter context
 * @param res pointer to return value of function
 * @return uint64_t true/false return
 */
char ubasic_builtin_str_fn(const char* fn_name, struct ubasic_ctx* ctx, char** res) {
	int i;
	for (i = 0; builtin_str[i].name; ++i) {
		if (!strcmp(fn_name, builtin_str[i].name)) {
			*res = builtin_str[i].handler(ctx);
			return 1;
		}
	}
	return 0;
}


int64_t ubasic_eval_int_fn(const char* fn_name, struct ubasic_ctx* ctx)
{
	struct ub_proc_fn_def* def = ubasic_find_fn(fn_name + 2, ctx);
	int64_t rv = 0;
	if (def != NULL)
	{
		ctx->gosub_stack_ptr++;

		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct ubasic_ctx* atomic = ubasic_clone(ctx);
		atomic->fn_type = RT_INT;
		jump_linenum(def->line, atomic);

		while (!ubasic_finished(atomic))
		{
			line_statement(atomic);
		}
		if (atomic->fn_return == NULL)
			tokenizer_error_print(ctx, "End of function without returning value");
		else
		{
			rv = (int64_t)atomic->fn_return;
		}

		/* Only free the base struct! */
		kfree(atomic);

		ctx->gosub_stack_ptr--;

		return rv;
	}
	tokenizer_error_print(ctx, "No such integer FN");
	return 0;
}

void ubasic_eval_double_fn(const char* fn_name, struct ubasic_ctx* ctx, double* res)
{
	struct ub_proc_fn_def* def = ubasic_find_fn(fn_name + 2, ctx);
	if (def != NULL)
	{
		ctx->gosub_stack_ptr++;

		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct ubasic_ctx* atomic = ubasic_clone(ctx);
		atomic->fn_type = RT_FLOAT;
		jump_linenum(def->line, atomic);

		while (!ubasic_finished(atomic))
		{
			line_statement(atomic);
		}
		if (atomic->fn_return == NULL)
			tokenizer_error_print(ctx, "End of function without returning value");
		else
		{
			*res = *((double*)atomic->fn_return);
		}

		/* Only free the base struct! */
		kfree(atomic);

		ctx->gosub_stack_ptr--;

		return;
	}
	tokenizer_error_print(ctx, "No such real FN");
	*res = 0.0;
	return;
}

/**
 * @brief Returns true if 'varname' starts with FN
 * (is a function call)
 * 
 * @param varname variable name to check
 * @return char 1 if variable name is a function call, 0 if it is not
 */
char varname_is_function(const char* varname) {
	return (*varname == 'F' && *(varname + 1) == 'N');
}

/*---------------------------------------------------------------------------*/
const char* ubasic_get_string_variable(const char* var, struct ubasic_ctx* ctx)
{
	char* retv;
	int t = ubasic_builtin_str_fn(var, ctx, &retv);
	if (t)
		return retv;

	if (varname_is_function(var)) {
		const char* res = ubasic_eval_str_fn(var, ctx);
		return res;
	}

	struct ub_var_string* list[] = {
		ctx->local_string_variables[ctx->gosub_stack_ptr],
		ctx->str_variables
	};
	int j;

	for (j = 0; j < 2; j++)
	{
		//kprintf("Iter %d\n", j);
		struct ub_var_string* cur = list[j];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname))	{
				return cur->value;
			}
		}
	}

	char err[1024];
	sprintf(err, "No such variable '%s'", var);
	tokenizer_error_print(ctx, err);
	return "";
}

int64_t ubasic_get_int_variable(const char* var, struct ubasic_ctx* ctx)
{
	int64_t retv = 0;
	if (ubasic_builtin_int_fn(var, ctx, &retv)) {
		return retv;
	}
		
	if (varname_is_function(var)) {
		return ubasic_eval_int_fn(var, ctx);
	}

	struct ub_var_int* list[] = { 
		ctx->local_int_variables[ctx->gosub_stack_ptr],
		ctx->int_variables
	};
	int j;

	for (j = 0; j < 2; j++)	{
		struct ub_var_int* cur = list[j];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname))	{
				int64_t v = cur->value;
				/* If ERROR is read, it resets its value */
				if (!strcmp(var, "ERROR")) {
					ubasic_set_int_variable("ERROR", 0, ctx, false, false);
				}
				return v;
			}
		}
	}


	char err[1024];
	sprintf(err, "No such variable '%s'", var);
	tokenizer_error_print(ctx, err);
	return 0; /* No such variable */
}

void ubasic_get_double_variable(const char* var, struct ubasic_ctx* ctx, double* res)
{
	if (ubasic_builtin_double_fn(var, ctx, res)) {
		return;
	}
		
	if (varname_is_function(var)) {
		ubasic_eval_double_fn(var, ctx, res);
		return;
	}

	struct ub_var_double* list[] = { 
		ctx->local_double_variables[ctx->gosub_stack_ptr],
		ctx->double_variables
	};
	int j;

	for (j = 0; j < 2; j++)	{
		struct ub_var_double* cur = list[j];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname))	{
				*res = cur->value;
				return;
			}
		}
	}


	char err[1024];
	sprintf(err, "No such variable '%s'", var);
	tokenizer_error_print(ctx, err);
	*res = 0.0; /* No such variable */
}

