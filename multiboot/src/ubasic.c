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

#define DEBUG 0

#if DEBUG
#define DEBUG_PRINTF(...)  kprintf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#define ubasic_error(str) tokenizer_error_print(ctx, str)

#include <kernel.h>
#include <ubasic.h>
#include <tokenizer.h>
#include <kprintf.h>
#include <kmalloc.h>
#include <string.h>
#include <input.h>
#include <video.h>
#include <taskswitch.h>
#include <filesystem.h>

static int expr(struct ubasic_ctx* ctx);
static void line_statement(struct ubasic_ctx* ctx);
static void statement(struct ubasic_ctx* ctx);
static const char* str_expr(struct ubasic_ctx* ctx);
const char* str_varfactor(struct ubasic_ctx* ctx);
void ubasic_parse_fn(struct ubasic_ctx* ctx);

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
	{ NULL, NULL }
};

struct ubasic_str_fn builtin_str[] =
{
	{ ubasic_left, "LEFT$" },
	{ ubasic_chr, "CHR$" },
	{ ubasic_readstring, "READ$" },
	{ ubasic_getname, "GETNAME$" },
	{ NULL, NULL }
};

/*---------------------------------------------------------------------------*/
struct ubasic_ctx* ubasic_init(const char *program, console* cons)
{
	struct ubasic_ctx* ctx = (struct ubasic_ctx*)kmalloc(sizeof(struct ubasic_ctx));
	int i;

	ctx->current_token = TOKENIZER_ERROR;	
	ctx->int_variables = NULL;
	ctx->str_variables = NULL;
	for (i = 0; i < MAX_GOSUB_STACK_DEPTH; i++)
		ctx->local_int_variables[i] = NULL;
	for (i = 0; i < MAX_GOSUB_STACK_DEPTH; i++)
		ctx->local_string_variables[i] = NULL;
	ctx->cons = cons;
	ctx->oldlen = 0;
	ctx->errored = 0;
	ctx->fn_return = NULL;
	// We allocate 5000 bytes extra on the end of the program for EVAL space,
	// as EVAL appends to the prgram on lines 9998 and 9999.
	ctx->program_ptr = (char*)kmalloc(strlen(program) + 5000);
	strlcpy(ctx->program_ptr, program, strlen(program) + 5000);
	ctx->for_stack_ptr = ctx->gosub_stack_ptr = 0;
	ctx->defs = NULL;

	// Scan the program for functions and procedures

        tokenizer_init(ctx->program_ptr, ctx);

	ubasic_parse_fn(ctx);

	return ctx;
}

struct ubasic_ctx* ubasic_clone(struct ubasic_ctx* old)
{
	struct ubasic_ctx* ctx = (struct ubasic_ctx*)kmalloc(sizeof(struct ubasic_ctx));
	int i;

	ctx->current_token = TOKENIZER_ERROR;
	ctx->int_variables = old->int_variables;
	ctx->str_variables = old->str_variables;

	for (i = 0; i < MAX_GOSUB_STACK_DEPTH; i++)
		ctx->local_int_variables[i] = old->local_int_variables[i];
	for (i = 0; i < MAX_GOSUB_STACK_DEPTH; i++)
		ctx->local_string_variables[i] = old->local_string_variables[i];

	ctx->cons = old->cons;
	ctx->errored = 0;
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
		currentline = tokenizer_num(ctx);
		char* linestart = ctx->ptr;
		do
		{
			do
			{
				tokenizer_next(ctx);
			}
			while (tokenizer_token(ctx) != TOKENIZER_CR && tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT);
			
			char* lineend = ctx->ptr;
			
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
				else if (!strncmp(search, "PROC", 2))
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

			if (tokenizer_token(ctx) == TOKENIZER_ENDOFINPUT)
			{
				break;
			}
		}
		while (tokenizer_token(ctx) != TOKENIZER_NUMBER && tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT);

		if (tokenizer_token(ctx) == TOKENIZER_ENDOFINPUT)
			break;
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
	if (token != tokenizer_token(ctx))
	{
		//kprintf("Expected %d got %d\n", token, tokenizer_token(ctx));
		tokenizer_error_print(ctx, "No such keyword");
	}
	tokenizer_next(ctx);
}
/*---------------------------------------------------------------------------*/
static int varfactor(struct ubasic_ctx* ctx)
{
	const char* vn = tokenizer_variable_name(ctx);
	int r = ubasic_get_int_variable(vn, ctx);
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

const char* str_varfactor(struct ubasic_ctx* ctx)
{
	const char* r;
	if (*ctx->ptr == '"')
	{
		tokenizer_string(ctx->string, sizeof(ctx->string), ctx);
		if (tokenizer_token(ctx) == TOKENIZER_RIGHTPAREN)
			accept(TOKENIZER_RIGHTPAREN, ctx);
		else
			accept(TOKENIZER_STRING, ctx);
		return ctx->string;
	}
	else
	{
		const char* vn = tokenizer_variable_name(ctx);
		r = ubasic_get_string_variable(vn, ctx);
		if (tokenizer_token(ctx) == TOKENIZER_RIGHTPAREN)
			accept(TOKENIZER_RIGHTPAREN, ctx);
		else
			accept(TOKENIZER_VARIABLE, ctx);
	}
	return r;
}

/*---------------------------------------------------------------------------*/
static int factor(struct ubasic_ctx* ctx)
{
      int r;

	//kprintf("factor: token %d\n", tokenizer_token(ctx));
	switch(tokenizer_token(ctx))
	{
		case TOKENIZER_NUMBER:
			r = tokenizer_num(ctx);
			//kprintf("factor: number %d\n", r);
			accept(TOKENIZER_NUMBER, ctx);
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
static int term(struct ubasic_ctx* ctx)
{
	int f1, f2;
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
/*---------------------------------------------------------------------------*/
static int expr(struct ubasic_ctx* ctx)
{
	int t1, t2;
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


/*---------------------------------------------------------------------------*/
void jump_linenum(int linenum, struct ubasic_ctx* ctx)
{
	tokenizer_init(ctx->program_ptr, ctx);
	
	while (tokenizer_num(ctx) != linenum)
	{
		do
		{
			do
			{
				tokenizer_next(ctx);
			}
			while (tokenizer_token(ctx) != TOKENIZER_CR && tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT);

			if (tokenizer_token(ctx) == TOKENIZER_CR)
			{
				tokenizer_next(ctx);
			}
		}
		while(tokenizer_token(ctx) != TOKENIZER_NUMBER);

		DEBUG_PRINTF("jump_linenum: Found line %d\n", tokenizer_num(ctx));
	}
}
/*---------------------------------------------------------------------------*/
static void goto_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_GOTO, ctx);
	jump_linenum(tokenizer_num(ctx), ctx);
}

static void colour_statement(struct ubasic_ctx* ctx, int tok)
{
	accept(tok, ctx);
	setforeground(ctx->cons, expr(ctx));
	accept(TOKENIZER_CR, ctx);
}

/*---------------------------------------------------------------------------*/
static void print_statement(struct ubasic_ctx* ctx)
{
	int numprints = 0;
	int no_newline = 0;

	accept(TOKENIZER_PRINT, ctx);

	do
	{
		no_newline = 0;
		DEBUG_PRINTF("Print loop\n");
		if (tokenizer_token(ctx) == TOKENIZER_STRING)
    		{
			tokenizer_string(ctx->string, sizeof(ctx->string), ctx);
			kprintf("%s", ctx->string);
			tokenizer_next(ctx);
		}
    		else if (tokenizer_token(ctx) == TOKENIZER_COMMA)
		{
			kprintf(" ");
			tokenizer_next(ctx);
		}
    		else if (tokenizer_token(ctx) == TOKENIZER_SEMICOLON)
		{
			no_newline = 1;
			tokenizer_next(ctx);
		}
    		else if (tokenizer_token(ctx) == TOKENIZER_PLUS)
		{
			tokenizer_next(ctx);
		}
		else if (tokenizer_token(ctx) == TOKENIZER_VARIABLE || tokenizer_token(ctx) == TOKENIZER_NUMBER)
		{
			{
				/* Check if it's a string or numeric expression */
				const char* oldctx = ctx->ptr;
				if (tokenizer_token(ctx) != TOKENIZER_NUMBER && (*ctx->ptr == '"' || strchr(tokenizer_variable_name(ctx), '$')))
				{
					ctx->ptr = oldctx;
					kprintf("%s", str_expr(ctx));
				}
		 		else
				{
					ctx->ptr = oldctx;
					kprintf("%d", expr(ctx));
				}
			}
		}
    		else
		{
			break;
		}
		numprints++;
	}
  	while(tokenizer_token(ctx) != TOKENIZER_CR && tokenizer_token(ctx) != TOKENIZER_ENDOFINPUT && numprints < 255);
  
	if (!no_newline)
		kprintf("\n");

	DEBUG_PRINTF("End of print\n");
	tokenizer_next(ctx);
}
/*---------------------------------------------------------------------------*/
static void if_statement(struct ubasic_ctx* ctx)
{
	int r;
  
	accept(TOKENIZER_IF, ctx);

	r = relation(ctx);
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
	proc_wait(proc_cur(), p->pid);
	accept(TOKENIZER_CR, ctx);
}

static void eval_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_EVAL, ctx);
	const char* v = str_expr(ctx);
	accept(TOKENIZER_CR, ctx);

	if (ctx->oldlen == 0)
	{
		ubasic_set_string_variable("ERROR$", "", ctx, 0);
		ubasic_set_int_variable("ERROR", 0, ctx, 0);
		ctx->oldlen = strlen(ctx->program_ptr);
		strlcat(ctx->program_ptr, "9998 ", ctx->oldlen + 5000);
		strlcat(ctx->program_ptr, v, ctx->oldlen + 5000);
		strlcat(ctx->program_ptr, "\n9999 RETURN\n", ctx->oldlen + 5000);

		ctx->eval_linenum = ctx->current_linenum;

		ctx->gosub_stack[ctx->gosub_stack_ptr++] = ctx->current_linenum;

		jump_linenum(9998, ctx);
	}
	else
	{
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

	if (kinput(10240, ctx->cons) != 0)
	{
		switch (var[strlen(var) - 1])
		{
			case '$':
				ubasic_set_string_variable(var, kgetinput(ctx->cons), ctx, 0);
			break;
			default:
				ubasic_set_int_variable(var, atoi(kgetinput(ctx->cons)), ctx, 0);
			break;
		}

		kfreeinput(ctx->cons);

		accept(TOKENIZER_CR, ctx);
	}
	else
	{
		jump_linenum(ctx->current_linenum, ctx);
	}
}

/*---------------------------------------------------------------------------*/
static void let_statement(struct ubasic_ctx* ctx)
{
	const char* var;
	const char* _expr;

	var = gc_strdup(tokenizer_variable_name(ctx));
	accept(TOKENIZER_VARIABLE, ctx);
	accept(TOKENIZER_EQ, ctx);

	//kprintf("let '%s' = ", var);

	switch (var[strlen(var) - 1])
	{
		case '$':
			_expr = str_expr(ctx);
			//kprintf(var);
			ubasic_set_string_variable(var, _expr, ctx, 0);
			//kprintf("'%s'\n", _expr);
		break;
		default:
			ubasic_set_int_variable(var, expr(ctx), ctx, 0);
			//kprintf("numeric\n");
		break;
	}
	accept(TOKENIZER_CR, ctx);
}
/*---------------------------------------------------------------------------*/
static void gosub_statement(struct ubasic_ctx* ctx)
{
	int linenum;

	accept(TOKENIZER_GOSUB, ctx);
	linenum = tokenizer_num(ctx);
	accept(TOKENIZER_NUMBER, ctx);
	accept(TOKENIZER_CR, ctx);

	if (ctx->gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH)
	{
		ctx->gosub_stack[ctx->gosub_stack_ptr] = tokenizer_num(ctx);
		ctx->gosub_stack_ptr++;
		jump_linenum(linenum, ctx);
	}
	else
	{
		tokenizer_error_print(ctx, "gosub_statement: gosub stack exhausted\n");
	}
}
/*---------------------------------------------------------------------------*/
static void return_statement(struct ubasic_ctx* ctx)
{
	//kprintf("Return\n");
	accept(TOKENIZER_RETURN, ctx);
	if (ctx->gosub_stack_ptr > 0)
	{
		ctx->gosub_stack_ptr--;
		jump_linenum(ctx->gosub_stack[ctx->gosub_stack_ptr], ctx);
	}
	else
	{
		tokenizer_error_print(ctx, "return_statement: non-matching return\n");
	}
}
/*---------------------------------------------------------------------------*/
static void next_statement(struct ubasic_ctx* ctx)
{
	accept(TOKENIZER_NEXT, ctx);
	if (ctx->for_stack_ptr > 0)
	{
		int incr = ubasic_get_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ctx);
		//kprintf("incr is %d\n", incr);
		ubasic_set_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ++incr, ctx, 0);

		if (incr < ctx->for_stack[ctx->for_stack_ptr - 1].to)
		{
			jump_linenum(ctx->for_stack[ctx->for_stack_ptr - 1].line_after_for, ctx);
		}
		else
		{
			kfree(ctx->for_stack[ctx->for_stack_ptr].for_variable);
			ctx->for_stack_ptr--;
			accept(TOKENIZER_CR, ctx);
		}
	}
	else
	{
		tokenizer_error_print(ctx, "next_statement: non-matching next\n");
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
	ubasic_set_int_variable(for_variable, expr(ctx), ctx, 0);
	int v = ubasic_get_int_variable(for_variable, ctx);
	//kprintf("Initial var is %d\n", v);
	accept(TOKENIZER_TO, ctx);
	to = expr(ctx);
	accept(TOKENIZER_CR, ctx);

	if (ctx->for_stack_ptr < MAX_FOR_STACK_DEPTH)
	{
		ctx->for_stack[ctx->for_stack_ptr].line_after_for = tokenizer_num(ctx);
		ctx->for_stack[ctx->for_stack_ptr].for_variable = for_variable;
		ctx->for_stack[ctx->for_stack_ptr].to = to;
		DEBUG_PRINTF("for_statement: new for, var %s to %d\n", ctx->for_stack[ctx->for_stack_ptr].for_variable, ctx->for_stack[ctx->for_stack_ptr].to); 
		ctx->for_stack_ptr++;
	}
	else
	{
		tokenizer_error_print(ctx, "for_statement: for stack depth exceeded\n");
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

	if (ctx->fn_type == RT_STRING)
		ctx->fn_return = (void*)str_expr(ctx);
	else
		ctx->fn_return = (void*)expr(ctx);

	accept(TOKENIZER_CR, ctx);

	ctx->ended = 1;
}

/*---------------------------------------------------------------------------*/
static void statement(struct ubasic_ctx* ctx)
{
	int token;
  
	token = tokenizer_token(ctx);

	switch (token)
	{
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
		case TOKENIZER_LET:
			accept(TOKENIZER_LET, ctx);
			/* Fall through. */
		case TOKENIZER_VARIABLE:
			let_statement(ctx);
		break;
		case TOKENIZER_EQ:
			eq_statement(ctx);
		break;
		default:
			tokenizer_error_print(ctx, "Unknown keyword\n");
			//kprintf("Bad token %d (%c) %c\n", token, token, ctx->current_token);
		break;
	}
}
/*---------------------------------------------------------------------------*/
static void line_statement(struct ubasic_ctx* ctx)
{
	ctx->errored = 0;
	ctx->current_linenum = tokenizer_num(ctx);
	DEBUG_PRINTF("----------- Line number %d ---------\n", ctx->current_linenum);
	accept(TOKENIZER_NUMBER, ctx);
	//kprintf("%s\n", ctx->ptr);
	statement(ctx);
	return;
}
/*---------------------------------------------------------------------------*/
void ubasic_run(struct ubasic_ctx* ctx)
{
	if (ubasic_finished(ctx))
	{
		//kprintf("End of program: '%s' %c\n", ctx->program_ptr, *(ctx->ptr - 2));
		DEBUG_PRINTF("uBASIC program finished\n");
		return;
	}
	line_statement(ctx);
	gc();
}
/*---------------------------------------------------------------------------*/
int ubasic_finished(struct ubasic_ctx* ctx)
{
	int st = ctx->ended || tokenizer_finished(ctx);
	//kprintf("finish: st=%d ended=%d tokfin=%d\n", st, ctx->ended, tokenizer_finished(ctx));
	return st;
}
/*---------------------------------------------------------------------------*/
void ubasic_set_variable(const char* var, const char* value, struct ubasic_ctx* ctx)
{
	/* first, itdentify the variable's type. Look for $ for string,
	 * and parenthesis for arrays
	 */
	const char last_letter = var[strlen(var) - 1];

	switch (last_letter)
	{
		case '$':
			ubasic_set_string_variable(var, value, ctx, 0);
		break;
		case ')':
			ubasic_set_array_variable(var, atoi(value), ctx, 0);
		break;
		default:
			ubasic_set_int_variable(var, atoi(value), ctx, 0);
		break;
	}
}

int valid_string_var(const char* name)
{
	const char* i;
	//kprintf("check var '%s'\n", name);
	if (strlen(name) < 2)
		return 0;
	if (name[strlen(name) - 1] != '$')
		return 0;
	for (i = name; *i != '$'; i++)
	{
		if (*i == '$' && *(i + 1) != 0)
		       return 0;	
	}
	if ((*name >='A' && *name <= 'Z') || (*name >= 'a' && *name <= 'z'))
	{
		return 1;
	}
	else return 0;
}

int valid_int_var(const char* name)
{
	return 1;
}

void ubasic_set_string_variable(const char* var, const char* value, struct ubasic_ctx* ctx, int local)
{
	struct ub_var_int* list[] = {ctx->str_variables, ctx->local_string_variables[ctx->gosub_stack_ptr]};

	//kprintf("set string '%s' to '%s' %d\n", var, value, local);

	if (!valid_string_var(var))
	{
		tokenizer_error_print(ctx, "Malformed variable name\n");
		return;
	}
	if (list[local] == NULL)
	{
		if (local)
		{
			ctx->local_string_variables[ctx->gosub_stack_ptr] = (struct ub_var_string*)kmalloc(sizeof(struct ub_var_string));
			ctx->local_string_variables[ctx->gosub_stack_ptr]->next = NULL;
			ctx->local_string_variables[ctx->gosub_stack_ptr]->varname = strdup(var);
			ctx->local_string_variables[ctx->gosub_stack_ptr]->value = strdup(value);
		}
		else
		{
			ctx->str_variables = (struct ub_var_string*)kmalloc(sizeof(struct ub_var_string));
			ctx->str_variables->next = NULL;
			ctx->str_variables->varname = strdup(var);
			ctx->str_variables->value = strdup(value);
		}
		return;
	}
	else
	{
		struct ub_var_string* cur = ctx->str_variables;
		if (local)
			cur = ctx->local_string_variables[ctx->gosub_stack_ptr];

		for (; cur; cur = cur->next)
		{
			if (!strcmp(var, cur->varname))
			{
				kfree(cur->value);
				cur->value = strdup(value);
				return;
			}
		}
		struct ub_var_string* newvar = (struct ub_var_string*)kmalloc(sizeof(struct ub_var_string));
		if (local)
			newvar->next = ctx->local_string_variables[ctx->gosub_stack_ptr];
		else
			newvar->next = ctx->str_variables;

		newvar->next = ctx->str_variables;
		newvar->varname = strdup(var);
		newvar->value = strdup(value);
	
		if (local)
			ctx->local_string_variables[ctx->gosub_stack_ptr] = newvar;
		else
			ctx->str_variables = newvar;
	}
}

void ubasic_set_array_variable(const char* var, int value, struct ubasic_ctx* ctx, int local)
{
}

void ubasic_set_int_variable(const char* var, int value, struct ubasic_ctx* ctx, int local)
{
	struct ub_var_int* list[] = {ctx->int_variables, ctx->local_int_variables[ctx->gosub_stack_ptr]};

	if (!valid_int_var(var))
	{
		tokenizer_error_print(ctx, "Malformed variable name\n");
		return;
	}

	if (list[local] == NULL)
	{
		if (local)
		{
			ctx->local_int_variables[ctx->gosub_stack_ptr] = (struct ub_var_int*)kmalloc(sizeof(struct ub_var_int));
			ctx->local_int_variables[ctx->gosub_stack_ptr]->next = NULL;
			ctx->local_int_variables[ctx->gosub_stack_ptr]->varname = strdup(var);
			ctx->local_int_variables[ctx->gosub_stack_ptr]->value = value;
		}
		else
		{
			ctx->int_variables = (struct ub_var_int*)kmalloc(sizeof(struct ub_var_int));
			ctx->int_variables->next = NULL;
			ctx->int_variables->varname = strdup(var);
			ctx->int_variables->value = value;
		}
		return;
	}
	else
	{
		struct ub_var_int* cur = ctx->int_variables;
		if (local)
			cur = ctx->local_int_variables[ctx->gosub_stack_ptr];
		for (; cur; cur = cur->next)
		{
			if (!strcmp(var, cur->varname))
			{
				cur->value = value;
				return;
			}
		}
		struct ub_var_int* newvar = (struct ub_var_int*)kmalloc(sizeof(struct ub_var_int));
		if (local)
			newvar->next = ctx->local_int_variables[ctx->gosub_stack_ptr];
		else
			newvar->next = ctx->int_variables;
		newvar->varname = strdup(var);
		newvar->value = value;
		if (local)
			ctx->local_int_variables[ctx->gosub_stack_ptr] = newvar;
		else
			ctx->int_variables = newvar;
	}
}

static int bracket_depth = 0;
char* item_begin = NULL;
struct ub_param* param = NULL;

void begin_comma_list(struct ub_proc_fn_def* def, struct ubasic_ctx* ctx)
{
	bracket_depth = 0;
	param = def->params;
	item_begin = ctx->ptr;
}



const char* extract_comma_list(struct ub_proc_fn_def* def, struct ubasic_ctx* ctx)
{
	if (*ctx->ptr == '(')
	{
		bracket_depth++;
		if (bracket_depth == 1)
			item_begin = ctx->ptr + 1;
	}
	else if (*ctx->ptr == ')')
		bracket_depth--;

	if ((*ctx->ptr == ',' && bracket_depth == 1) || (*ctx->ptr == ')' && bracket_depth == 0))
	{
		// next item
		// set local vars here
		// Set ctx to item_begin, call expr(), set ctx back again. Change expr to stop on comma.
		//
		// XXX We know wether to call expr or str_expr here based upon the type for the fn param
		// which we will read when this is implemented. We should probably check the fn exists
		// before we even GET here!
		char oldval = *ctx->ptr;
		char oldct = ctx->current_token;
		char* oldptr = ctx->ptr;
		char* oldnextptr = ctx->nextptr;
		ctx->nextptr = item_begin;
		ctx->ptr = item_begin;
		ctx->current_token = get_next_token(ctx);
		*oldptr = 0;
		//kprintf("*** Calling with '%s'\n", ctx->ptr);
		if (param)
		{
			//kprintf("Setting parameter '%s'\n", param->name);

			if (param->name[strlen(param->name) - 1] == '$')
			{
				const char* val = str_expr(ctx);
				//kprintf("String expr val '%s'\n", val);
				ubasic_set_string_variable(param->name, val, ctx, 1);
			}
			else
			{
				int val = expr(ctx);
				//kprintf("Int expr val '%d'\n", val);
				ubasic_set_int_variable(param->name, val, ctx, 1);
			}

			param = param->next;
		}
		*oldptr = oldval;
		ctx->ptr = oldptr;
		ctx->nextptr = oldnextptr;
		ctx->current_token = oldct;
		item_begin = ctx->ptr + 1;
	}

	ctx->ptr++;

	if (bracket_depth == 0 || *ctx->ptr == 0)
	{
		ctx->nextptr = ctx->ptr;
		//ctx->current_token = get_next_token(ctx);
		//kprintf("current token %d\n", ctx->current_token);
		return NULL;
	}

	return 1;

}

const char* ubasic_eval_str_fn(const char* fn_name, struct ubasic_ctx* ctx)
{
	struct ub_proc_fn_def* def = ubasic_find_fn(fn_name + 2, ctx);
	const char* rv = gc_strdup("");
	if (def != NULL)
	{
		ctx->gosub_stack_ptr++;
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct ubasic_ctx* atomic = ubasic_clone(ctx);
		atomic->fn_type = RT_STRING;
		jump_linenum(def->line, atomic);

		while (!ubasic_finished(atomic))
		{
			line_statement(atomic);
		}
		if (atomic->fn_return == NULL)
			tokenizer_error_print(ctx, "End of function without returning value");
		else
		{
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
#define PARAMS_START int itemtype = BIP_INT; int intval; char* strval; char* oldval; char oldct; char* oldptr; char* oldnextptr; int gotone = 0; int bracket_depth = 0; char* item_begin = ctx->ptr;
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
			oldptr = ctx->ptr; \
			oldnextptr = ctx->nextptr; \
			ctx->nextptr = item_begin; \
			ctx->ptr = item_begin; \
			ctx->current_token = get_next_token(ctx); \
			*oldptr = 0; \
			itemtype = type; \
			if (itemtype == BIP_STRING) \
			{ \
				strval = str_expr(ctx); \
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
		tokenizer_error_print(ctx, "Too many parameters for function " NAME ); \
}

int ubasic_openin(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	return _open(strval, _O_RDONLY);
}

int ubasic_getnamecount(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	FS_DirectoryEntry* fsl = fs_get_items(strval);
	int count = 0;
	while (fsl)
	{
		fsl = fsl->next;
		count++;
	}
	return count;
}

char* ubasic_getname(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	FS_DirectoryEntry* fsl = fs_get_items(strval);
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

int ubasic_getsize(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	FS_DirectoryEntry* fsl = fs_get_items(strval);
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

int ubasic_eof(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	return _eof(intval);
}

int ubasic_asc(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	return *strval;
}

char* ubasic_chr(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	char res[2] = {(char)intval, 0};
	return gc_strdup(res);
}

int ubasic_instr(struct ubasic_ctx* ctx)
{
	int i;
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

int ubasic_read(struct ubasic_ctx* ctx)
{
	char res;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	if (_read(intval, &res, 1) != 1)
		tokenizer_error_print(ctx, "Error reading from file");
	return res;
}

char* ubasic_left(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	if (intval > strlen(strval))
		intval = strlen(strval);
	char* cut = gc_strdup(strval);
	*(cut + intval) = 0;
	return cut;
}

int ubasic_len(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	return strlen(strval);
}

int ubasic_abs(struct ubasic_ctx* ctx)
{
	PARAMS_START;

	PARAMS_GET_ITEM(BIP_INT);
	int v = intval;

	PARAMS_END("ABS");

	if (v < 0)
		return 0 + -v;
	else
		return v;
}

int ubasic_builtin_int_fn(const char* fn_name, struct ubasic_ctx* ctx, int* res)
{
	int i;
	for (i = 0; builtin_int[i].name; ++i)
	{
		if (!strcmp(fn_name, builtin_int[i].name))
		{
			*res = builtin_int[i].handler(ctx);
			return 1;
		}
	}
	return 0;
}

int ubasic_builtin_str_fn(const char* fn_name, struct ubasic_ctx* ctx, char** res)
{
	int i;
	for (i = 0; builtin_str[i].name; ++i)
	{
		if (!strcmp(fn_name, builtin_str[i].name))
		{
			*res = builtin_str[i].handler(ctx);
			return 1;
		}
	}
	return 0;
}


int ubasic_eval_int_fn(const char* fn_name, struct ubasic_ctx* ctx)
{
	struct ub_proc_fn_def* def = ubasic_find_fn(fn_name + 2, ctx);
	int rv = 0;
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
			rv = (int)atomic->fn_return;
		}

		/* Only free the base struct! */
		kfree(atomic);

		ctx->gosub_stack_ptr--;

		return rv;
	}
	tokenizer_error_print(ctx, "No such integer FN");
	return 0;
}

/*---------------------------------------------------------------------------*/
const char* ubasic_get_string_variable(const char* var, struct ubasic_ctx* ctx)
{
	char* retv;
	int t = ubasic_builtin_str_fn(var, ctx, &retv);
	if (t)
		return retv;

	if (var[0] == 'F' && var[1] == 'N')
	{
		const char* res = ubasic_eval_str_fn(var, ctx);
		return res;
	}

	//kprintf("Get '%s'\n", var);

	struct ub_var_string* list[] = {ctx->local_string_variables[ctx->gosub_stack_ptr], ctx->str_variables};
	int j;

	for (j = 0; j < 2; j++)
	{
		//kprintf("Iter %d\n", j);
		struct ub_var_string* cur = list[j];
		for (; cur; cur = cur->next)
		{
			//kprintf("'%s'=%s\n", cur->varname, cur->value);
			if (!strcmp(var, cur->varname))
			{
				return cur->value;
			}
		}
	}

	tokenizer_error_print(ctx, "No such variable\n");
	return "";
}

int ubasic_get_int_variable(const char* var, struct ubasic_ctx* ctx)
{
	int retv;
	int t = ubasic_builtin_int_fn(var, ctx, &retv);
	if (t)
		return retv;
		
	if (var[0] == 'F' && var[1] == 'N')
	{
		int res = ubasic_eval_int_fn(var, ctx);
		return res;
	}

	struct ub_var_int* list[] = {ctx->local_int_variables[ctx->gosub_stack_ptr], ctx->int_variables};
	int j;

	for (j = 0; j < 2; j++)
	{
		struct ub_var_int* cur = list[j];
		for (; cur; cur = cur->next)
		{
			if (!strcmp(var, cur->varname))
			{
				return cur->value;
			}
		}
	}


	tokenizer_error_print(ctx, "No such variable\n");
	return 0; /* No such variable */
}

