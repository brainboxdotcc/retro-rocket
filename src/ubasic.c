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

#include <kernel.h>

void begin_comma_list(struct ub_proc_fn_def* def, struct ubasic_ctx* ctx);
uint8_t extract_comma_list(struct ub_proc_fn_def* def, struct ubasic_ctx* ctx);
bool conditional(struct ubasic_ctx* ctx);

struct ubasic_int_fn builtin_int[] =
{
	{ ubasic_abs, "ABS" },
	{ ubasic_len, "LEN" },
	{ ubasic_openin, "OPENIN" },
	{ ubasic_openout, "OPENOUT" },
	{ ubasic_openup, "OPENUP" },
	{ ubasic_eof, "EOF" },
	{ ubasic_read, "READ" },
	{ ubasic_instr, "INSTR" },
	{ ubasic_asc, "ASC" },
	{ ubasic_getnamecount, "GETNAMECOUNT" },
	{ ubasic_getsize, "GETSIZE" },
	{ ubasic_getproccount, "GETPROCCOUNT" },
	{ ubasic_getprocid, "GETPROCID" },
	{ ubasic_getprocparent, "GETPROCPARENT" },
	{ ubasic_getproccpuid, "GETPROCCPUID" },
	{ ubasic_rgb, "RGB" },
	{ ubasic_get_text_max_x, "TERMWIDTH" },
	{ ubasic_get_text_max_y, "TERMHEIGHT" },
	{ ubasic_get_text_cur_x, "CURRENTX" },
	{ ubasic_get_text_cur_y, "CURRENTY" },
	{ ubasic_get_free_mem, "MEMFREE" },
	{ ubasic_get_used_mem, "MEMUSED" },
	{ ubasic_get_total_mem, "MEMORY" },
	{ ubasic_sockstatus, "SOCKSTATUS" },
	{ ubasic_ctrlkey, "CTRLKEY" },
	{ ubasic_shiftkey, "SHIFTKEY" },
	{ ubasic_altkey, "ALTKEY" },
	{ ubasic_capslock, "ALTKEY" },
	{ NULL, NULL }
};

struct ubasic_double_fn builtin_double[] = {
	{ ubasic_sin, "SIN" },
	{ ubasic_cos, "COS" },
	{ ubasic_tan, "TAN" },
	{ ubasic_pow, "POW" },
	{ NULL, NULL },
};

struct ubasic_str_fn builtin_str[] =
{
	{ ubasic_netinfo, "NETINFO$" },
	{ ubasic_dns, "DNS$" },
	{ ubasic_left, "LEFT$" },
	{ ubasic_mid, "MID$" },
	{ ubasic_chr, "CHR$" },
	{ ubasic_insocket, "INSOCKET$" },
	{ ubasic_readstring, "READ$" },
	{ ubasic_getname, "GETNAME$" },
	{ ubasic_getprocname, "GETPROCNAME$" },
	{ ubasic_ramdisk_from_device, "RAMDISK$" },
	{ ubasic_ramdisk_from_size, "RAMDISK" },
	{ ubasic_inkey, "INKEY$" },
	{ ubasic_upper, "UPPER$" },
	{ ubasic_lower, "LOWER$" },
	{ ubasic_tokenize, "TOKENIZE$" },
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

const char* auto_number(const char* program, uint64_t line, uint64_t increment)
{
	size_t new_size_max = strlen(program) * 5;
	char* newprog = kmalloc(new_size_max);
	char line_buffer[MAX_STRINGLEN];
	char* line_ptr = line_buffer;
	bool insert_line = true, ended = false;
	*newprog = 0;
	while (true) {
		if (insert_line) {
			while (*program == '\n') {
				*line_ptr++ = '\n';
				program++;
			}
			snprintf(line_buffer, MAX_STRINGLEN, "%d ", line);
			line += increment;
			insert_line = false;
			line_ptr = line_buffer + strlen(line_buffer);
			if (ended) {
				break;
			}
		}
		if (*program == '\n' || !*program) {
			if (!*program) {
				ended = true;
			}
			insert_line = true;
			*line_ptr = 0;
			strlcat(newprog, line_buffer, new_size_max);
			strlcat(newprog, "\n", new_size_max);
		}
		*line_ptr++ = *program++;
	}
	strlcat(newprog, "\n", new_size_max);
	const char* corrected = strdup(newprog);
	kfree(newprog);
	return corrected;
}

uint64_t line_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const ub_line_ref *l = item;
	return l->line_number * l->line_number * seed0 * seed1;
}

int line_compare(const void *a, const void *b, void *udata) {
	const ub_line_ref *la = a;
	const ub_line_ref *lb = b;
	return la->line_number == lb->line_number ? 0 : (la->line_number < lb->line_number ? -1 : 1);
}

struct ubasic_ctx* ubasic_init(const char *program, console* cons, uint32_t pid, const char* file, char** error)
{
	if (!isdigit(*program)) {
		/* Program is not line numbered! Auto-number it. */
		const char* numbered = auto_number(program, 10, 10);
		struct ubasic_ctx* c = ubasic_init(numbered, cons, pid, file, error);
		kfree(numbered);
		return c;
	}
	struct ubasic_ctx* ctx = kmalloc(sizeof(struct ubasic_ctx));
	if (ctx == NULL) {
		*error = "Out of memory";
		return NULL;
	}
	int i;

	ctx->if_nest_level = 0;
	ctx->errored = false;
	ctx->current_token = ERROR;
	ctx->int_variables = NULL;
	ctx->str_variables = NULL;
	ctx->fn_type = RT_MAIN;
	ctx->double_variables = NULL;
	ctx->int_array_variables = NULL;
	ctx->string_array_variables = NULL;
	ctx->double_array_variables = NULL;
	for (i = 0; i < MAX_CALL_STACK_DEPTH; i++)
		ctx->local_int_variables[i] = NULL;
	for (i = 0; i < MAX_CALL_STACK_DEPTH; i++)
		ctx->local_string_variables[i] = NULL;
	for (i = 0; i < MAX_CALL_STACK_DEPTH; i++)
		ctx->local_double_variables[i] = NULL;
	ctx->cons = (struct console*)cons;
	ctx->oldlen = 0;
	ctx->fn_return = NULL;
	// We allocate 5000 bytes extra on the end of the program for EVAL space,
	// as EVAL appends to the program on lines EVAL_LINE and EVAL_LINE + 1.
	ctx->program_ptr = kmalloc(strlen(program) + 5000);
	if (ctx->program_ptr == NULL) {
		kfree(ctx);
		*error = "Out of memory";
		return NULL;
	}
	ctx->lines = hashmap_new(sizeof(ub_line_ref), 0, 5923530135432, 458397058, line_hash, line_compare, NULL, NULL);

	// Clean extra whitespace from the program
	ctx->program_ptr = clean_basic(program, ctx->program_ptr);

	ctx->for_stack_ptr = ctx->call_stack_ptr = ctx->repeat_stack_ptr = 0;
	ctx->defs = NULL;

	ctx->graphics_colour = 0xFFFFFF;

	// Scan the program for functions and procedures

	tokenizer_init(ctx->program_ptr, ctx);
	ubasic_parse_fn(ctx);

	set_system_variables(ctx, pid);
	ubasic_set_string_variable("PROGRAM$", file, ctx, false, false);

	/* Build doubly linked list of line number references */
	uint64_t last_line = 0xFFFFFFFF;
	while (!tokenizer_finished(ctx)) {
		uint64_t line = tokenizer_num(ctx, NUMBER);
		if (last_line != 0xFFFFFFFF && (line <= last_line)) {
			*error = "Misordered lines in BASIC program";
			ubasic_destroy(ctx);
			return NULL;
		}
		last_line = line;
		if (hashmap_set(ctx->lines, &(ub_line_ref){ .line_number = line, .ptr = ctx->ptr })) {
			*error = "Line hashed twice in BASIC program (internal error)";
			ubasic_destroy(ctx);
			return NULL;
		}
		do {
			do {
				tokenizer_next(ctx);
			}
			while (tokenizer_token(ctx) != NEWLINE && !tokenizer_finished(ctx));
			if (tokenizer_token(ctx) == NEWLINE) {
				tokenizer_next(ctx);
			}
		}
		while(tokenizer_token(ctx) != NUMBER && !tokenizer_finished(ctx));
	}
	tokenizer_init(ctx->program_ptr, ctx);

	return ctx;
}

struct ubasic_ctx* ubasic_clone(struct ubasic_ctx* old)
{
	struct ubasic_ctx* ctx = kmalloc(sizeof(struct ubasic_ctx));
	int i;

	ctx->if_nest_level = old->if_nest_level;
	ctx->current_token = ERROR;
	ctx->int_variables = old->int_variables;
	ctx->str_variables = old->str_variables;
	ctx->double_variables = old->double_variables;
	ctx->int_array_variables = old->int_array_variables;
	ctx->string_array_variables = old->string_array_variables;
	ctx->double_array_variables = old->double_array_variables;
	ctx->lines = old->lines;

	for (i = 0; i < MAX_CALL_STACK_DEPTH; i++)
		ctx->local_int_variables[i] = old->local_int_variables[i];
	for (i = 0; i < MAX_CALL_STACK_DEPTH; i++)
		ctx->local_string_variables[i] = old->local_string_variables[i];
	for (i = 0; i < MAX_CALL_STACK_DEPTH; i++)
		ctx->local_double_variables[i] = old->local_double_variables[i];

	for (i = 0; i < MAX_LOOP_STACK_DEPTH; i++) {
		ctx->for_stack[i] = old->for_stack[i];
		ctx->repeat_stack[i] = old->repeat_stack[i];
	}

	ctx->cons = old->cons;
	ctx->oldlen = old->oldlen;
	ctx->fn_return = NULL;
	ctx->program_ptr = old->program_ptr;
	ctx->for_stack_ptr = old->for_stack_ptr;
	ctx->call_stack_ptr = old->call_stack_ptr;
	ctx->repeat_stack_ptr = old->repeat_stack_ptr;
	ctx->defs = old->defs;
	ctx->ended = false;
	ctx->errored = false;

	tokenizer_init(ctx->program_ptr, ctx);

	return ctx;
}

void ubasic_parse_fn(struct ubasic_ctx* ctx)
{
	int currentline = 0;

	while (true) {
		currentline = tokenizer_num(ctx, NUMBER);
		char const* linestart = ctx->ptr;
		do {
			do {
				tokenizer_next(ctx);
			} while (tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT);
			
			char const* lineend = ctx->ptr;
			
			char* linetext = kmalloc(lineend - linestart + 1);
			strlcpy(linetext, linestart, lineend - linestart + 1);

			char* search = linetext;

			while (*search++ >= '0' && *search <= '9')
				search++;
			--search;

			while (*search++ == ' ');
			--search;

			if (!strncmp(search, "DEF ", 4)) {
				search += 4;
				ub_fn_type type = FT_FN;
				if (!strncmp(search, "FN", 2)) {
					search += 2;
					while (*search++ == ' ');
					type = FT_FN;
				} else if (!strncmp(search, "PROC", 4)) {
					search += 4;
					while (*search++ == ' ');
					type = FT_PROC;
				}

				char name[MAX_STRINGLEN];
				int ni = 0;
				struct ub_proc_fn_def* def = kmalloc(sizeof(struct ub_proc_fn_def));
				--search;
				while (ni < MAX_STRINGLEN - 2 && *search != '\n' && *search != 0 && *search != '(') {
					name[ni++] = *search++;
				}
				name[ni] = 0;

				def->name = strdup(name);
				def->type = type;
				def->line = currentline;
				def->next = ctx->defs;

				dprintf("PROC: %s (line %d)\n", def->name, currentline);

				/* Parse parameters */

				def->params = NULL;

				if (*search == '(') {
					search++;
					// Parse parameters
					char pname[MAX_STRINGLEN];
					int pni = 0;
					while (*search != 0 && *search != '\n') {
						if (pni < MAX_STRINGLEN - 1 && *search != ',' && *search != ')' && *search != ' ') {
							pname[pni++] = *search;
						}

						if (*search == ',' || *search == ')') {
							pname[pni] = 0;
							struct ub_param* par = kmalloc(sizeof(struct ub_param));

							par->next = NULL;
							par->name = strdup(pname);

							if (def->params == NULL) {
								def->params = par;
							} else {
								struct ub_param* cur = def->params;
								for (; cur; cur = cur->next) {
									if (cur->next == NULL) {
										cur->next = par;
										dprintf("   PARAM: %s\n", cur->name);
										break;
									}
								}
							}
							if (*search == ')') {
								break;
							}
							pni = 0;
						}
						search++;
					}
				}
				ctx->defs = def;
			}

			if (tokenizer_token(ctx) == NEWLINE) {
				tokenizer_next(ctx);
			}
			kfree(linetext);
			if (tokenizer_token(ctx) == ENDOFINPUT) {
				break;
			}
		}
		while (tokenizer_token(ctx) != NUMBER && tokenizer_token(ctx) != ENDOFINPUT);

		if (tokenizer_token(ctx) == ENDOFINPUT) {
			break;
		}
	}

	tokenizer_init(ctx->program_ptr, ctx);

	ctx->ended = false;
	return;
}

struct ub_proc_fn_def* ubasic_find_fn(const char* name, struct ubasic_ctx* ctx)
{
	struct ub_proc_fn_def* cur = ctx->defs;
	for (; cur; cur = cur->next) {
		if (!strcmp(name, cur->name)) {
			return cur;
		}
	}
	return NULL;
}

void ubasic_destroy(struct ubasic_ctx* ctx)
{
	for (; ctx->int_variables; ctx->int_variables = ctx->int_variables->next) {
		kfree(ctx->int_variables->varname);
		kfree(ctx->int_variables);
	}
	for (; ctx->double_variables; ctx->double_variables = ctx->double_variables->next) {
		kfree(ctx->double_variables->varname);
		kfree(ctx->double_variables);
	}
	for (; ctx->str_variables; ctx->str_variables = ctx->str_variables->next) {
		kfree(ctx->str_variables->varname);
		kfree(ctx->str_variables->value);
		kfree(ctx->str_variables);
	}
	for (; ctx->int_array_variables; ctx->int_array_variables = ctx->int_array_variables->next) {
		kfree(ctx->int_array_variables->varname);
		kfree(ctx->int_array_variables->values);
		kfree(ctx->int_array_variables);
	}
	for (; ctx->double_array_variables; ctx->double_array_variables = ctx->double_array_variables->next) {
		kfree(ctx->double_array_variables->varname);
		kfree(ctx->double_array_variables->values);
		kfree(ctx->double_array_variables);
	}
	for (; ctx->string_array_variables; ctx->string_array_variables = ctx->string_array_variables->next) {
		kfree(ctx->string_array_variables->varname);
		for (size_t f = 0; f < ctx->string_array_variables->itemcount; ++f) {
			if (ctx->string_array_variables->values[f]) {
				kfree(ctx->string_array_variables->values[f]);
			}
		}
		kfree(ctx->string_array_variables);
	}
	for (size_t x = 0; x < ctx->call_stack_ptr; x++) {
		for (; ctx->local_int_variables[x]; ctx->local_int_variables[x] = ctx->local_int_variables[x]->next) {
			kfree(ctx->local_int_variables[x]->varname);
			kfree(ctx->local_int_variables[x]);
		}
		for (; ctx->local_double_variables[x]; ctx->local_double_variables[x] = ctx->local_double_variables[x]->next) {
			kfree(ctx->local_double_variables[x]->varname);
			kfree(ctx->local_double_variables[x]);
		}
		for (; ctx->local_string_variables[x]; ctx->local_string_variables[x] = ctx->local_string_variables[x]->next) {
			kfree(ctx->local_string_variables[x]->varname);
			kfree(ctx->local_string_variables[x]->value);
			kfree(ctx->local_string_variables[x]);
		}
	}
	hashmap_free(ctx->lines);
	for (; ctx->defs; ctx->defs = ctx->defs->next) {
		kfree(ctx->defs->name);
		for (; ctx->defs->params; ctx->defs->params = ctx->defs->params->next) {
			kfree(ctx->defs->params->name);
			kfree(ctx->defs->params);
		}
	}
	kfree((char*)ctx->program_ptr);
	kfree(ctx);
}

void accept(int token, struct ubasic_ctx* ctx)
{
	if (token != tokenizer_token(ctx)) {
		char err[MAX_STRINGLEN];
		GENERATE_ENUM_STRING_NAMES(TOKEN, token_names)
		sprintf(err, "Expected %s got %s", token_names[token], token_names[tokenizer_token(ctx)]);
		tokenizer_error_print(ctx, err);
		return;
	}
	tokenizer_next(ctx);
}

bool jump_linenum(int64_t linenum, struct ubasic_ctx* ctx)
{
	ub_line_ref* line = hashmap_get(ctx->lines, &(ub_line_ref){ .line_number = linenum });
	if (!line) {
		char err[MAX_STRINGLEN];
		snprintf(err, MAX_STRINGLEN, "No such line %d", linenum);
		tokenizer_error_print(ctx, err);
		return false;
	}
	ctx->ptr = line->ptr;
	ctx->current_token = get_next_token(ctx);
	return true;
}

void goto_statement(struct ubasic_ctx* ctx)
{
	accept(GOTO, ctx);
	jump_linenum(tokenizer_num(ctx, NUMBER), ctx);
}

void colour_statement(struct ubasic_ctx* ctx, int tok)
{
	accept(tok, ctx);
	setforeground((console*)ctx->cons, expr(ctx));
	accept(NEWLINE, ctx);
}

void background_statement(struct ubasic_ctx* ctx)
{
	accept(BACKGROUND, ctx);
	setbackground((console*)ctx->cons, expr(ctx));
	accept(NEWLINE, ctx);
}

bool is_builtin_double_fn(const char* fn_name) {
	for (int i = 0; builtin_double[i].name; ++i) {
		if (!strcmp(fn_name, builtin_double[i].name)) {
			return true;
		}
	}
	return false;
}

/**
 * @brief Free variables held on the local call stack
 * 
 * @param ctx BASIC context
 */
void free_local_heap(struct ubasic_ctx* ctx)
{
	while (ctx->local_string_variables[ctx->call_stack_ptr]) {
		struct ub_var_string* next = ctx->local_string_variables[ctx->call_stack_ptr]->next;
		kfree(ctx->local_string_variables[ctx->call_stack_ptr]->value);
		kfree(ctx->local_string_variables[ctx->call_stack_ptr]->varname);
		kfree(ctx->local_string_variables[ctx->call_stack_ptr]);
		ctx->local_string_variables[ctx->call_stack_ptr] = next;
	}
	while (ctx->local_int_variables[ctx->call_stack_ptr]) {
		struct ub_var_int* next = ctx->local_int_variables[ctx->call_stack_ptr]->next;
		kfree(ctx->local_int_variables[ctx->call_stack_ptr]->varname);
		kfree(ctx->local_int_variables[ctx->call_stack_ptr]);
		ctx->local_int_variables[ctx->call_stack_ptr] = next;
	}
	while (ctx->local_double_variables[ctx->call_stack_ptr]) {
		struct ub_var_double* next = ctx->local_double_variables[ctx->call_stack_ptr]->next;
		kfree(ctx->local_double_variables[ctx->call_stack_ptr]->varname);
		kfree(ctx->local_double_variables[ctx->call_stack_ptr]);
		ctx->local_double_variables[ctx->call_stack_ptr] = next;
	}
	ctx->local_int_variables[ctx->call_stack_ptr] = NULL;
	ctx->local_string_variables[ctx->call_stack_ptr] = NULL;
	ctx->local_double_variables[ctx->call_stack_ptr] = NULL;
}

/**
 * @brief Initialise the local call stack
 * 
 * @param ctx 
 */
void init_local_heap(struct ubasic_ctx* ctx)
{
	ctx->local_int_variables[ctx->call_stack_ptr] = NULL;
	ctx->local_string_variables[ctx->call_stack_ptr] = NULL;
	ctx->local_double_variables[ctx->call_stack_ptr] = NULL;
}


char* printable_syntax(struct ubasic_ctx* ctx)
{
	int numprints = 0;
	int no_newline = 0;
	int next_hex = 0;
	char buffer[MAX_STRINGLEN], out[MAX_STRINGLEN];
	
	*out = 0;

	do {
		no_newline = 0;
		*buffer = 0;
		if (tokenizer_token(ctx) == STRING) {
			if (tokenizer_string(ctx->string, sizeof(ctx->string), ctx)) {
				snprintf(buffer, MAX_STRINGLEN, "%s", ctx->string);
				strlcat(out, buffer, MAX_STRINGLEN);
			} else {
				tokenizer_error_print(ctx, "Unterminated \"");
				return NULL;
			}
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == COMMA) {
			strlcat(out, "\t", MAX_STRINGLEN);
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == SEMICOLON) {
			no_newline = 1;
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == TILDE) {
			next_hex = 1;
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == PLUS) {
			tokenizer_next(ctx);
		} else if (tokenizer_token(ctx) == VARIABLE || tokenizer_token(ctx) == NUMBER || tokenizer_token(ctx) == HEXNUMBER) {
			/* Check if it's a string or numeric expression */
			const char* oldctx = ctx->ptr;
			if (tokenizer_token(ctx) != NUMBER && tokenizer_token(ctx) != HEXNUMBER && (*ctx->ptr == '"' || strchr(tokenizer_variable_name(ctx), '$'))) {
				ctx->ptr = oldctx;
				strlcpy(buffer, str_expr(ctx), MAX_STRINGLEN);
				strlcat(out, buffer, MAX_STRINGLEN);
			} else {
				ctx->ptr = oldctx;
				const char* var_name = tokenizer_variable_name(ctx);
				ctx->ptr = oldctx;
				bool printable_double = (strchr(var_name, '#') != NULL || is_builtin_double_fn(var_name) || tokenizer_decimal_number(ctx));
				if (printable_double) {
					double f = 0.0;
					char double_buffer[32];
					ctx->ptr = oldctx;
					double_expr(ctx, &f);
					strlcat(buffer, double_to_string(f, double_buffer, 32, 0), MAX_STRINGLEN);
				} else {
					ctx->ptr = oldctx;
					snprintf(buffer, MAX_STRINGLEN, next_hex ? "%lX" : "%ld", expr(ctx));
				}
				strlcat(out, buffer, MAX_STRINGLEN);
				next_hex = 0;
			}
		} else {
			break;
		}
		numprints++;
	}
  	while(tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT && numprints < 255);
  
	if (!no_newline) {
		strlcat(out, "\n", MAX_STRINGLEN);
	}

	tokenizer_next(ctx);

	if (ctx->errored) {
		return NULL;
	}
	return gc_strdup(out);
}

void print_statement(struct ubasic_ctx* ctx)
{
	accept(PRINT, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		putstring((console*)ctx->cons, out);
	}
}

void sockwrite_statement(struct ubasic_ctx* ctx)
{
	int fd = -1;

	accept(SOCKWRITE, ctx);
	fd = ubasic_get_numeric_int_variable(tokenizer_variable_name(ctx), ctx);
	accept(VARIABLE, ctx);
	accept(COMMA, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		send(fd, out, strlen(out));
	}
}

void proc_statement(struct ubasic_ctx* ctx)
{
	char procname[MAX_STRINGLEN];
	char* p = procname;
	size_t procnamelen = 0;
	accept(PROC, ctx);
	while (*ctx->ptr != '\n' && *ctx->ptr != 0  && *ctx->ptr != '(' && procnamelen < MAX_STRINGLEN - 1) {
		if (*ctx->ptr != ' ') {
			*(p++) = *(ctx->ptr++);
		}
		procnamelen++;
	}
	*p++ = 0;
	struct ub_proc_fn_def* def = ubasic_find_fn(procname, ctx);
	if (def) {
		if (*ctx->ptr == '(' && *(ctx->ptr + 1) != ')') {
			ctx->call_stack_ptr++;
			init_local_heap(ctx);
			begin_comma_list(def, ctx);
			while (extract_comma_list(def, ctx));
			ctx->call_stack_ptr--;
		} else {
			ctx->call_stack_ptr++;
			init_local_heap(ctx);
			ctx->call_stack_ptr--;
		}
		ctx->fn_type = RT_NONE;

		while (tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT) {
			tokenizer_next(ctx);
		}
		accept(NEWLINE, ctx);

		if (ctx->call_stack_ptr < MAX_CALL_STACK_DEPTH) {
			ctx->call_stack[ctx->call_stack_ptr] = tokenizer_num(ctx, NUMBER);
			ctx->call_stack_ptr++;
			jump_linenum(def->line, ctx);
		} else {
			tokenizer_error_print(ctx, "PROC: stack exhausted");
		}
		return;
	}
	char err[MAX_STRINGLEN];
	snprintf(err, MAX_STRINGLEN, "No such PROC %s", procname);
}

bool conditional(struct ubasic_ctx* ctx)
{
	char current_line[MAX_STRINGLEN];
	char* pos = strchr(ctx->ptr, '\n');
	char* end = strchr(ctx->ptr, 0);
	bool stringlike = false, real = false;
	strlcpy(current_line, ctx->ptr, pos ? pos - ctx->ptr + 1 : end - ctx->ptr + 1);
	for (char* n = current_line; *n && *n != '\n'; ++n) {
		if (strlen(n) >= 2 && isalnum(*n) && *(n + 1) == '$') {
			stringlike = true; /* String variable */
			break;
		} else if (strlen(n) >= 2 && isalnum(*n) && *(n + 1) == '#') {
			real = true; /* Real variable */
			break;
		} else if (strlen(n) >= 3 && isdigit(*n) && *(n + 1) == '.' && isdigit(*(n + 2))) {
			real = true; /* Decimal number */
			break;
		} else if (strlen(n) >= 5 && *n == ' ' && *(n + 1) == 'T' && *(n + 2) == 'H' && *(n + 3) == 'E' && *(n + 4) == 'N') {
			break;
		} else if (*n == '\n') {
			break;
		}
	}

	bool r = false;
	if (!real) {
		int64_t ri = stringlike ? str_relation(ctx) :  relation(ctx);
		r = (ri != 0);
	} else {
		double rd;
		double_relation(ctx, &rd);
		r = (rd != 0.0);
	}
	return r;
}

void else_statement(struct ubasic_ctx* ctx)
{
	/* If we get to an ELSE, this means that we executed a THEN part of a block IF,
	 * so we must skip it and any content up until the next ENDIF 
	 */
	accept(ELSE, ctx);
	accept(NEWLINE, ctx);
	while (tokenizer_token(ctx) != END && !tokenizer_finished(ctx)) {
		tokenizer_next(ctx);
		if (*ctx->ptr == 'I' && *(ctx->ptr+1) == 'F') {
			accept(IF, ctx);
			accept(NEWLINE, ctx);
			return;
		}
	}
	tokenizer_error_print(ctx, "Block IF/THEN/ELSE without ENDIF");
}

void if_statement(struct ubasic_ctx* ctx)
{
	accept(IF, ctx);
	bool r = conditional(ctx);
	accept(THEN, ctx);
	if (r) {
		if (tokenizer_token(ctx) == NEWLINE) {
			/* Multi-statement block IF */
			accept(NEWLINE, ctx);
			ctx->if_nest_level++;
			return;
		}
		statement(ctx);
	} else {
		if (tokenizer_token(ctx) == NEWLINE) {
			/* Multi-statement block IF, looking for ELSE */
			const char* old_start = ctx->ptr;
			const char* old_end = ctx->nextptr;
			while (!tokenizer_finished(ctx)) {
				int t = tokenizer_token(ctx);
				tokenizer_next(ctx);
				if (t == ELSE && tokenizer_token(ctx) == NEWLINE) {
					ctx->if_nest_level++;
					accept(NEWLINE, ctx);
					return;
				}
			}
			/* Didn't find a multiline ELSE, look for ENDIF */
			ctx->ptr = old_start;
			ctx->nextptr = old_end;
			ctx->current_token = 0;
			while (!tokenizer_finished(ctx)) {
				int t = tokenizer_token(ctx);
				tokenizer_next(ctx);
				if (t == END && tokenizer_token(ctx) == IF) {
					accept(IF, ctx);
					accept(NEWLINE, ctx);
					return;
				}
			}
		}
		do {
			tokenizer_next(ctx);
		} while (tokenizer_token(ctx) != ELSE && tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT);

		if (tokenizer_token(ctx) == ELSE) {
			tokenizer_next(ctx);
			statement(ctx);
		} else if (tokenizer_token(ctx) == NEWLINE) {
			tokenizer_next(ctx);
		}
	}
}

void chain_statement(struct ubasic_ctx* ctx)
{
	accept(CHAIN, ctx);
	const char* pn = str_expr(ctx);
	process_t* p = proc_load(pn, ctx->cons, proc_cur()->pid);
	if (p == NULL) {
		accept(NEWLINE, ctx);
		return;
	}
	struct ubasic_ctx* new_proc = p->code;
	struct ub_var_int* cur_int = ctx->int_variables;
	struct ub_var_string* cur_str = ctx->str_variables;
	struct ub_var_double* cur_double = ctx->double_variables;

	/* Inherit global variables into new process */
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

	proc_wait(proc_cur(), p->pid);
	accept(NEWLINE, ctx);
}

void eval_statement(struct ubasic_ctx* ctx)
{
	accept(EVAL, ctx);
	const char* v = str_expr(ctx);
	accept(NEWLINE, ctx);

	if (ctx->current_linenum == EVAL_LINE) {
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
		/* If program doesn't end in newline, add one */
		char last = ctx->program_ptr[strlen(ctx->program_ptr) - 1];
		if (last > 13) {
			strlcat(ctx->program_ptr, "\n", ctx->oldlen + 5000);
		}
		const char* line_eval = (ctx->program_ptr + strlen(ctx->program_ptr));
		strlcat(ctx->program_ptr, STRINGIFY(EVAL_LINE)" ", ctx->oldlen + 5000);
		strlcat(ctx->program_ptr, clean_v, ctx->oldlen + 5000);
		const char* line_eval_end = (ctx->program_ptr + strlen(ctx->program_ptr) + 1);
		strlcat(ctx->program_ptr, "\n"STRINGIFY(EVAL_END_LINE)" RETURN\n", ctx->oldlen + 5000);

		hashmap_set(ctx->lines, &(ub_line_ref){ .line_number = EVAL_LINE, .ptr = line_eval });
		hashmap_set(ctx->lines, &(ub_line_ref){ .line_number = EVAL_END_LINE, .ptr = line_eval_end });

		/**
		 * @brief An EVAL jumps to the line number at the end of the program
		 * (EVAL_LINE) where the evaluation is to take place. Once compeleted,
		 * a RETURN statement is executed at EVAL_END_LINE, which returns back
		 * to the line where the EVAL statement is.
		 */
		ctx->eval_linenum = ctx->current_linenum;
		ctx->call_stack[ctx->call_stack_ptr] = ctx->current_linenum;
		ctx->call_stack_ptr++;
		init_local_heap(ctx);

		jump_linenum(EVAL_LINE, ctx);
	} else {
		ctx->program_ptr[ctx->oldlen] = 0;
		ctx->oldlen = 0;
		ctx->eval_linenum = 0;
		/* Delete references to the eval lines */
		hashmap_delete(ctx->lines, &(ub_line_ref){ .line_number = EVAL_LINE });
		hashmap_delete(ctx->lines, &(ub_line_ref){ .line_number = EVAL_END_LINE });
	}
}

void rem_statement(struct ubasic_ctx* ctx)
{
	accept(REM, ctx);
	while (tokenizer_token(ctx) != ENDOFINPUT && tokenizer_token(ctx) != NEWLINE) {
		tokenizer_next(ctx);
	}
	accept(NEWLINE, ctx);
}

void def_statement(struct ubasic_ctx* ctx)
{
	// Because the function or procedure definition is pre-parsed by ubasic_init(),
	// we just skip the entire line moving to the next if we hit a DEF statement.
	// in the future we should check if the interpreter is actually calling a FN,
	// to check we dont fall through into a function.
	accept(DEF, ctx);
	while (tokenizer_token(ctx) != ENDOFINPUT && tokenizer_token(ctx) != NEWLINE) {
		tokenizer_next(ctx);
	}
	accept(NEWLINE, ctx);
}

/**
 * @brief Process INPUT statement.
 * 
 * The INPUT statement will yield while waiting for input, essentially if
 * there is no complete line in the input buffer yet, it will yield back to
 * the OS task loop for other processes to get a turn. Each time the process
 * is entered while waiting for the input line to be completed, it will just
 * loop back against the same INPUT statement again until completed.
 * 
 * @param ctx BASIC context
 */
void input_statement(struct ubasic_ctx* ctx)
{
	accept(INPUT, ctx);
	const char* var = tokenizer_variable_name(ctx);
	accept(VARIABLE, ctx);

	/* Clear buffer */
	if (kinput(10240, (console*)ctx->cons) != 0) {
		switch (var[strlen(var) - 1]) {
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
		accept(NEWLINE, ctx);
	} else {
		jump_linenum(ctx->current_linenum, ctx);
	}
}

/**
 * @brief Process SOCKREAD statement.
 * 
 * The SOCKREAD statement will yield while waiting for data, essentially if
 * there is no complete line in the data buffer yet, it will yield back to
 * the OS task loop for other processes to get a turn. Each time the process
 * is entered while waiting for the data to be completed, it will just
 * loop back against the same SOCKREAD statement again until completed.
 * 
 * @param ctx BASIC context
 */
void sockread_statement(struct ubasic_ctx* ctx)
{
	char input[MAX_STRINGLEN];
	const char* var = NULL;
	int fd = -1;

	accept(SOCKREAD, ctx);
	fd = ubasic_get_numeric_int_variable(tokenizer_variable_name(ctx), ctx);
	accept(VARIABLE, ctx);
	accept(COMMA, ctx);
	var = tokenizer_variable_name(ctx);
	accept(VARIABLE, ctx);

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

		accept(NEWLINE, ctx);
	} else if (rv < 0) {
		tokenizer_error_print(ctx, socket_error(rv));
	} else {
		jump_linenum(ctx->current_linenum, ctx);
	}
}

void connect_statement(struct ubasic_ctx* ctx)
{
	char input[MAX_STRINGLEN];
	const char* fd_var = NULL, *ip = NULL;
	int64_t port = 0;

	accept(CONNECT, ctx);
	fd_var = tokenizer_variable_name(ctx);
	accept(VARIABLE, ctx);
	accept(COMMA, ctx);
	ip = str_expr(ctx);
	accept(COMMA, ctx);
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

		accept(NEWLINE, ctx);
	} else {
		tokenizer_error_print(ctx, socket_error(rv));
	}
}

void sockclose_statement(struct ubasic_ctx* ctx)
{
	const char* fd_var = NULL;

	accept(SOCKCLOSE, ctx);
	fd_var = tokenizer_variable_name(ctx);
	accept(VARIABLE, ctx);

	int rv = closesocket(ubasic_get_numeric_int_variable(fd_var, ctx));
	if (rv == 0) {
		// Clear variable to -1
		ubasic_set_int_variable(fd_var, -1, ctx, false, false);
		accept(NEWLINE, ctx);
	} else {
		tokenizer_error_print(ctx, socket_error(rv));
	}
}

void let_statement(struct ubasic_ctx* ctx, bool global, bool local)
{
	const char* var;
	const char* _expr;
	double f_expr = 0;

	var = tokenizer_variable_name(ctx);

	if (varname_is_int_array_access(ctx, var)) {
		int64_t index = arr_expr_set_index(ctx, var);
		int64_t value = expr(ctx);
		if (index == -1) {
			ubasic_set_int_array(var, value, ctx);
			accept(NEWLINE, ctx);
			return;
		}
		ubasic_set_int_array_variable(var, index, value, ctx);
		accept(NEWLINE, ctx);
		return;
	}
	if (varname_is_string_array_access(ctx, var)) {
		int64_t index = arr_expr_set_index(ctx, var);
		const char* value = str_expr(ctx);
		if (index == -1) {
			ubasic_set_string_array(var, value, ctx);
			accept(NEWLINE, ctx);
			return;
		}
		ubasic_set_string_array_variable(var, index, value, ctx);
		accept(NEWLINE, ctx);
		return;
	}
	if (varname_is_double_array_access(ctx, var)) {
		int64_t index = arr_expr_set_index(ctx, var);
		double value = 0;
		double_expr(ctx, &value);
		if (index == -1) {
			ubasic_set_double_array(var, value, ctx);
			accept(NEWLINE, ctx);
			return;
		}
		ubasic_set_double_array_variable(var, index, value, ctx);
		accept(NEWLINE, ctx);
		return;
	}

	accept(VARIABLE, ctx);
	accept(EQUALS, ctx);

	switch (var[strlen(var) - 1])
	{
		case '$':
			_expr = str_expr(ctx);
			ubasic_set_string_variable(var, _expr, ctx, local, global);
		break;
		case '#':
			double_expr(ctx, &f_expr);
			ubasic_set_double_variable(var, f_expr, ctx, local, global);
		break;
		default:
			ubasic_set_int_variable(var, expr(ctx), ctx, local, global);
		break;
	}
	accept(NEWLINE, ctx);
}

void cls_statement(struct ubasic_ctx* ctx)
{
	accept(CLS, ctx);
	clearscreen(current_console);
	accept(NEWLINE, ctx);
}

void gcol_statement(struct ubasic_ctx* ctx)
{
	accept(GCOL, ctx);
	ctx->graphics_colour = expr(ctx);
	//dprintf("New graphics color: %08X\n", ctx->graphics_colour);
	accept(NEWLINE, ctx);
}

void gotoxy_statement(struct ubasic_ctx* ctx)
{
	accept(CURSOR, ctx);
	int64_t x = expr(ctx);
	accept(COMMA, ctx);
	int64_t y = expr(ctx);
	gotoxy(x, y);
	accept(NEWLINE, ctx);
}

void draw_line_statement(struct ubasic_ctx* ctx)
{
	accept(LINE, ctx);
	int64_t x1 = expr(ctx);
	accept(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept(COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept(COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept(NEWLINE, ctx);
	draw_line(x1, y1, x2, y2, ctx->graphics_colour);
}

void point_statement(struct ubasic_ctx* ctx)
{
	accept(POINT, ctx);
	int64_t x1 = expr(ctx);
	accept(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept(NEWLINE, ctx);
	putpixel(x1, y1, ctx->graphics_colour);
}

void triangle_statement(struct ubasic_ctx* ctx)
{
	accept(TRIANGLE, ctx);
	int64_t x1 = expr(ctx);
	accept(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept(COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept(COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept(COMMA, ctx);
	int64_t x3 = expr(ctx);
	accept(COMMA, ctx);
	int64_t y3 = expr(ctx);
	accept(NEWLINE, ctx);
	draw_triangle(x1, y1, x2, y2, x3, y3, ctx->graphics_colour);
}

void rectangle_statement(struct ubasic_ctx* ctx)
{
	accept(RECTANGLE, ctx);
	int64_t x1 = expr(ctx);
	accept(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept(COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept(COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept(NEWLINE, ctx);
	draw_horizontal_rectangle(x1, y1, x2, y2, ctx->graphics_colour);
}

void circle_statement(struct ubasic_ctx* ctx)
{
	accept(CIRCLE, ctx);
	int64_t x = expr(ctx);
	accept(COMMA, ctx);
	int64_t y = expr(ctx);
	accept(COMMA, ctx);
	int64_t radius = expr(ctx);
	accept(COMMA, ctx);
	int64_t filled = expr(ctx);
	accept(NEWLINE, ctx);
	draw_circle(x, y, radius, filled, ctx->graphics_colour);
}

void gosub_statement(struct ubasic_ctx* ctx)
{
	int linenum;

	accept(GOSUB, ctx);
	linenum = tokenizer_num(ctx, NUMBER);
	accept(NUMBER, ctx);
	accept(NEWLINE, ctx);

	if (ctx->call_stack_ptr < MAX_CALL_STACK_DEPTH) {
		ctx->call_stack[ctx->call_stack_ptr] = tokenizer_num(ctx, NUMBER);
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		jump_linenum(linenum, ctx);
	} else {
		tokenizer_error_print(ctx, "GOSUB: stack exhausted");
	}
}

void return_statement(struct ubasic_ctx* ctx)
{
	accept(RETURN, ctx);
	if (ctx->fn_type != RT_MAIN)  {
		tokenizer_error_print(ctx, "RETURN inside FN or PROC");
		return;
	}
	if (ctx->call_stack_ptr > 0) {
		free_local_heap(ctx);
		ctx->call_stack_ptr--;
		jump_linenum(ctx->call_stack[ctx->call_stack_ptr], ctx);
	} else {
		tokenizer_error_print(ctx, "RETURN without GOSUB");
	}
}

void next_statement(struct ubasic_ctx* ctx)
{
	accept(NEXT, ctx);
	if (ctx->for_stack_ptr > 0) {
		bool continue_loop = false;
		if (strchr(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, '#')) {
			double incr;
			ubasic_get_double_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ctx, &incr);
			incr += ctx->for_stack[ctx->for_stack_ptr - 1].step;
			ubasic_set_double_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, incr, ctx, false, false);
			continue_loop = ((ctx->for_stack[ctx->for_stack_ptr - 1].step > 0 && incr < ctx->for_stack[ctx->for_stack_ptr - 1].to) ||
			    (ctx->for_stack[ctx->for_stack_ptr - 1].step < 0 && incr > ctx->for_stack[ctx->for_stack_ptr - 1].to));
		} else {
			int64_t incr = ubasic_get_numeric_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ctx);
			incr += ctx->for_stack[ctx->for_stack_ptr - 1].step;
			ubasic_set_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, incr, ctx, false, false);
			continue_loop = ((ctx->for_stack[ctx->for_stack_ptr - 1].step > 0 && incr < ctx->for_stack[ctx->for_stack_ptr - 1].to) ||
			    (ctx->for_stack[ctx->for_stack_ptr - 1].step < 0 && incr > ctx->for_stack[ctx->for_stack_ptr - 1].to));
		}
		if (continue_loop) {
			jump_linenum(ctx->for_stack[ctx->for_stack_ptr - 1].line_after_for, ctx);
		} else {
			kfree(ctx->for_stack[ctx->for_stack_ptr].for_variable);
			ctx->for_stack_ptr--;
			accept(NEWLINE, ctx);
		}

	} else {
		tokenizer_error_print(ctx, "NEXT without FOR");
		accept(NEWLINE, ctx);
	}
}

void for_statement(struct ubasic_ctx* ctx)
{
	accept(FOR, ctx);
	const char* for_variable = strdup(tokenizer_variable_name(ctx));
	accept(VARIABLE, ctx);
	accept(EQUALS, ctx);
	if (strchr(for_variable, '#')) {
		ubasic_set_double_variable(for_variable, expr(ctx), ctx, false, false);
	} else {
		ubasic_set_int_variable(for_variable, expr(ctx), ctx, false, false);
	}
	accept(TO, ctx);
	/* STEP needs special treatment, as it happens after an expression and is not separated by a comma */
	char* marker = NULL;
	for (char* n = (char*)ctx->ptr; *n && *n != '\n'; ++n) {
		if (strncmp(n, " STEP ", 6) == 0) {
			*n = '\n';
			marker = n;
			break;
		}
	}
	uint64_t to = expr(ctx), step = 1;
	tokenizer_next(ctx);
	if (marker) {
		*marker = ' ';
	}
	if (tokenizer_token(ctx) == STEP) {
		accept(STEP, ctx);
		step = expr(ctx);
		accept(NEWLINE, ctx);
	}

	if (ctx->for_stack_ptr < MAX_LOOP_STACK_DEPTH) {
		ctx->for_stack[ctx->for_stack_ptr].line_after_for = tokenizer_num(ctx, NUMBER);
		ctx->for_stack[ctx->for_stack_ptr].for_variable = for_variable;
		ctx->for_stack[ctx->for_stack_ptr].to = to;
		ctx->for_stack[ctx->for_stack_ptr].step = step;
		ctx->for_stack_ptr++;
	} else {
		tokenizer_error_print(ctx, "Too many FOR");
	}
}

void repeat_statement(struct ubasic_ctx* ctx)
{
	accept(REPEAT, ctx);
	accept(NEWLINE, ctx);
	if (ctx->repeat_stack_ptr < MAX_LOOP_STACK_DEPTH) {
		ctx->repeat_stack[ctx->repeat_stack_ptr] = tokenizer_num(ctx, NUMBER);
		ctx->repeat_stack_ptr++;
	} else {
		tokenizer_error_print(ctx, "REPEAT stack exhausted");
	}

}


void until_statement(struct ubasic_ctx* ctx)
{
	accept(UNTIL, ctx);
	bool done = conditional(ctx);
	accept(NEWLINE, ctx);

	if (ctx->repeat_stack_ptr > 0) {
		if (!done) {
			jump_linenum(ctx->repeat_stack[ctx->repeat_stack_ptr - 1], ctx);
		} else {
			ctx->repeat_stack_ptr--;
		}
	} else {
		tokenizer_error_print(ctx, "UNTIL without REPEAT");
	}

}

void endif_statement(struct ubasic_ctx* ctx)
{
	if (ctx->if_nest_level == 0) {
		tokenizer_error_print(ctx, "ENDIF outside of block IF");
	}
	accept(IF, ctx);
	accept(NEWLINE, ctx);
	ctx->if_nest_level--;
}

void end_statement(struct ubasic_ctx* ctx)
{
	accept(END, ctx);
	if (tokenizer_token(ctx) == IF) {
		endif_statement(ctx);
		return;
	}
	ctx->ended = true;
}

void eq_statement(struct ubasic_ctx* ctx)
{
	accept(EQUALS, ctx);

	if (ctx->fn_type == RT_STRING) {
		ctx->fn_return = (void*)str_expr(ctx);
	} else if (ctx->fn_type == RT_FLOAT) {
		double_expr(ctx, (void*)&ctx->fn_return);
	} else if (ctx->fn_type == RT_INT)  {
		ctx->fn_return = (void*)expr(ctx);
	} else if (ctx->fn_type == RT_NONE)  {
		tokenizer_error_print(ctx, "Can't return a value from a PROC");
		return;
	}

	accept(NEWLINE, ctx);

	ctx->ended = true;
}

void retproc_statement(struct ubasic_ctx* ctx)
{
	accept(RETPROC, ctx);
	accept(NEWLINE, ctx);
	if (ctx->fn_type != RT_NONE && ctx->fn_type != RT_MAIN)  {
		tokenizer_error_print(ctx, "Can't RETPROC from a FN");
		return;
	}
	ctx->fn_type = RT_MAIN;
	if (ctx->call_stack_ptr > 0) {
		free_local_heap(ctx);
		ctx->call_stack_ptr--;
		jump_linenum(ctx->call_stack[ctx->call_stack_ptr], ctx);
	} else {
		tokenizer_error_print(ctx, "RETPROC when not inside PROC");
	}
}

int64_t ubasic_asc(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("ASC");
	return (unsigned char)*strval;
}

void statement(struct ubasic_ctx* ctx)
{
	int token = tokenizer_token(ctx);

	switch (token) {
		case REM:
			return rem_statement(ctx);
		case BACKGROUND:
			return background_statement(ctx);
		case COLOR:
			return colour_statement(ctx, COLOR);
		case COLOUR:
			return colour_statement(ctx, COLOUR);
		case DEF:
			return def_statement(ctx);
		case CHAIN:
			return chain_statement(ctx);
		case EVAL:
			return eval_statement(ctx);
		case OPENIN:
			return openin_statement(ctx);
		case OPENOUT:
			return openout_statement(ctx);
		case OPENUP:
			return openup_statement(ctx);
		case READ:
			return read_statement(ctx);
		case CLOSE:
			return close_statement(ctx);
		case EOF:
			return eof_statement(ctx);
		case PRINT:
			return print_statement(ctx);
		case PROC:
			return proc_statement(ctx);
		case RETPROC:
			return retproc_statement(ctx);
		case IF:
			return if_statement(ctx);
		case ELSE:
			return else_statement(ctx);
		case CURSOR:
			return gotoxy_statement(ctx);
		case GOTO:
			return goto_statement(ctx);
		case GOSUB:
			return gosub_statement(ctx);
		case REPEAT:
			return repeat_statement(ctx);
		case UNTIL:
			return until_statement(ctx);
		case RETURN:
			return return_statement(ctx);
		case FOR:
			return for_statement(ctx);
		case NEXT:
			return next_statement(ctx);
		case END:
			return end_statement(ctx);
		case INPUT:
			return input_statement(ctx);
		case SOCKREAD:
			return sockread_statement(ctx);
		case SOCKWRITE:
			return sockwrite_statement(ctx);
		case DELETE:
			return delete_statement(ctx);
		case RMDIR:
			return rmdir_statement(ctx);
		case MOUNT:
			return mount_statement(ctx);
		case MKDIR:
			return mkdir_statement(ctx);
		case CONNECT:
			return connect_statement(ctx);
		case SOCKCLOSE:
			return sockclose_statement(ctx);
		case POINT:
			return point_statement(ctx);
		case CLS:
			return cls_statement(ctx);
		case GCOL:
			return gcol_statement(ctx);
		case LINE:
			return draw_line_statement(ctx);
		case TRIANGLE:
			return triangle_statement(ctx);
		case WRITE:
			return write_statement(ctx);
		case RECTANGLE:
			return rectangle_statement(ctx);
		case DIM:
			return dim_statement(ctx);
		case REDIM:
			return redim_statement(ctx);
		case PUSH:
			return push_statement(ctx);
		case POP:
			return pop_statement(ctx);
		case CIRCLE:
			return circle_statement(ctx);
		case LET:
			accept(LET, ctx);
			/* Fall through. */
		case VARIABLE:
			return let_statement(ctx, false, false);
		case GLOBAL:
			accept(GLOBAL, ctx);
			return let_statement(ctx, true, false);
		case LOCAL:
			accept(LOCAL, ctx);
			return let_statement(ctx, false, true);
		case EQUALS:
			return eq_statement(ctx);
		default:
			return tokenizer_error_print(ctx, "Unknown keyword");
	}
}

void line_statement(struct ubasic_ctx* ctx)
{
	ctx->current_linenum = tokenizer_num(ctx, NUMBER);
	accept(NUMBER, ctx);
	statement(ctx);
}

void ubasic_run(struct ubasic_ctx* ctx)
{
	if (ubasic_finished(ctx)) {
		return;
	}
	line_statement(ctx);
	/*if (ctx->fn_type == RT_NONE) {
		dprintf("Line %d\n", ctx->current_linenum);
	}*/
	if (ctx->errored) {
		ctx->errored = false;
		if (ctx->call_stack_ptr > 0) {
			free_local_heap(ctx);
			ctx->call_stack_ptr--;
			if (jump_linenum(ctx->call_stack[ctx->call_stack_ptr], ctx)) {
				line_statement(ctx);
			}
		}
	}
	gc();
}

bool ubasic_finished(struct ubasic_ctx* ctx)
{
	return ctx->ended || tokenizer_finished(ctx);
}

bool valid_string_var(const char* name)
{
	const char* i;
	unsigned int varLength = strlen(name);
	if (varLength < 2 || name[varLength - 1] != '$') {
		return false;
	}
	for (i = name; *i != '$'; i++) {
		if (*i == '$' && *(i + 1) != 0) {
		       return false;
		}
		if ((*i < 'A' || *i > 'Z') && (*i < 'a' || *i > 'z')) {
			return false;
		}
	}
	return true;
}

bool valid_double_var(const char* name)
{
	const char* i;
	unsigned int varLength = strlen(name);
	if (varLength < 2 || name[varLength - 1] != '#') {
		return false;
	}
	for (i = name; *i != '#'; i++) {
		if (*i == '#' && *(i + 1) != 0) {
		       return false;
		}
		if ((*i < 'A' || *i > 'Z') && (*i < 'a' || *i > 'z')) {
			return false;
		}
	}
	return true;
}

bool valid_int_var(const char* name)
{
	const char* i;
	unsigned int varLength = strlen(name);
	if (varLength < 1) {
		return false;
	}
	for (i = name; *i; i++) {
		if ((*i < 'A' || *i > 'Z') && (*i < 'a' || *i > 'z')) {
			return false;
		}
	}
	return true;
}

void ubasic_set_string_variable(const char* var, const char* value, struct ubasic_ctx* ctx, bool local, bool global)
{
	bool error_set = false;
	struct ub_var_string* list[] = {
		ctx->str_variables,
		ctx->local_string_variables[ctx->call_stack_ptr]
	};

	if (*value && !strcmp(var, "ERROR$")) {
		error_set = true;
	}

	if (!valid_string_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}
	if (list[local] == NULL) {
		if (local) {
			ctx->local_string_variables[ctx->call_stack_ptr] = kmalloc(sizeof(struct ub_var_string));
			ctx->local_string_variables[ctx->call_stack_ptr]->next = NULL;
			ctx->local_string_variables[ctx->call_stack_ptr]->varname = strdup(var);
			ctx->local_string_variables[ctx->call_stack_ptr]->value = strdup(value);
			ctx->local_string_variables[ctx->call_stack_ptr]->global = global;
		} else {
			ctx->str_variables = kmalloc(sizeof(struct ub_var_string));
			ctx->str_variables->next = NULL;
			ctx->str_variables->varname = strdup(var);
			ctx->str_variables->value = strdup(value);
			ctx->str_variables->global = global;
		}
		return;
	} else {
		struct ub_var_string* cur = ctx->str_variables;
		if (local) {
			cur = ctx->local_string_variables[ctx->call_stack_ptr];
		}
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname))	{
				if (error_set && *cur->value) {
					/* If ERROR$ is set, can't change it except to empty */
					return;
				} else if (error_set) {
					dprintf("Set ERROR$ to: '%s'\n", value);
				}
				kfree(cur->value);
				cur->value = strdup(value);
				cur->global = global;
				return;
			}
		}
		struct ub_var_string* newvar = kmalloc(sizeof(struct ub_var_string));
		newvar->next = (local ? ctx->local_string_variables[ctx->call_stack_ptr] : ctx->str_variables);
		newvar->varname = strdup(var);
		newvar->value = strdup(value);
		newvar->global = global;
		if (local) {
			ctx->local_string_variables[ctx->call_stack_ptr] = newvar;
		} else {
			ctx->str_variables = newvar;
		}
	}
}

void ubasic_set_int_variable(const char* var, int64_t value, struct ubasic_ctx* ctx, bool local, bool global)
{
	struct ub_var_int* list[] = {
		ctx->int_variables,
		ctx->local_int_variables[ctx->call_stack_ptr]
	};

	if (!valid_int_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}

	if (list[local] == NULL) {
		if (local) {
			//dprintf("Set int variable '%s' to '%d' (gosub local)\n", var, value);
			ctx->local_int_variables[ctx->call_stack_ptr] = kmalloc(sizeof(struct ub_var_int));
			ctx->local_int_variables[ctx->call_stack_ptr]->next = NULL;
			ctx->local_int_variables[ctx->call_stack_ptr]->varname = strdup(var);
			ctx->local_int_variables[ctx->call_stack_ptr]->value = value;
		} else {
			//dprintf("Set int variable '%s' to '%d' (default)\n", var, value);
			ctx->int_variables = kmalloc(sizeof(struct ub_var_int));
			ctx->int_variables->next = NULL;
			ctx->int_variables->varname = strdup(var);
			ctx->int_variables->value = value;
		}
		return;
	} else {
		struct ub_var_int* cur = ctx->int_variables;
		if (local)
			cur = ctx->local_int_variables[ctx->call_stack_ptr];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname)) {
				//dprintf("Set int variable '%s' to '%d' (updating)\n", var, value);
				cur->value = value;
				return;
			}
		}
		//dprintf("Set int variable '%s' to '%d'\n", var, value);
		struct ub_var_int* newvar = kmalloc(sizeof(struct ub_var_int));
		newvar->next = (local ? ctx->local_int_variables[ctx->call_stack_ptr] : ctx->int_variables);
		newvar->varname = strdup(var);
		newvar->value = value;
		if (local) {
			ctx->local_int_variables[ctx->call_stack_ptr] = newvar;
		} else {
			ctx->int_variables = newvar;
		}
	}
}

void ubasic_set_double_variable(const char* var, double value, struct ubasic_ctx* ctx, bool local, bool global)
{
	struct ub_var_double* list[] = {
		ctx->double_variables,
		ctx->local_double_variables[ctx->call_stack_ptr]
	};
	//char buffer[MAX_STRINGLEN];

	if (!valid_double_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}

	if (list[local] == NULL) {
		if (local) {
			//dprintf("Set double variable '%s' to '%s' (gosub local)\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
			ctx->local_double_variables[ctx->call_stack_ptr] = kmalloc(sizeof(struct ub_var_double));
			ctx->local_double_variables[ctx->call_stack_ptr]->next = NULL;
			ctx->local_double_variables[ctx->call_stack_ptr]->varname = strdup(var);
			ctx->local_double_variables[ctx->call_stack_ptr]->value = value;
		} else {
			//dprintf("Set double variable '%s' to '%s' (default)\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
			ctx->double_variables = kmalloc(sizeof(struct ub_var_double));
			ctx->double_variables->next = NULL;
			ctx->double_variables->varname = strdup(var);
			ctx->double_variables->value = value;
		}
		return;
	} else {
		struct ub_var_double* cur = ctx->double_variables;
		if (local)
			cur = ctx->local_double_variables[ctx->call_stack_ptr];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname)) {
				//dprintf("Set double variable '%s' to '%s' (updating)\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
				cur->value = value;
				return;
			}
		}
		//dprintf("Set double variable '%s' to '%s'\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
		struct ub_var_double* newvar = kmalloc(sizeof(struct ub_var_double));
		newvar->next = (local ? ctx->local_double_variables[ctx->call_stack_ptr] : ctx->double_variables);
		newvar->varname = strdup(var);
		newvar->value = value;
		if (local) {
			ctx->local_double_variables[ctx->call_stack_ptr] = newvar;
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
			//dprintf("Set PROC/FN param: %s to ", ctx->param->name);
			if (ctx->param->name[len - 1] == '$') {
				//dprintf("string value\n");
				ubasic_set_string_variable(ctx->param->name, str_expr(ctx), ctx, true, false);
			} else if (ctx->param->name[len - 1] == '#') {
				double f = 0.0;
				double_expr(ctx, &f);
				//dprintf("double value\n");
				ubasic_set_double_variable(ctx->param->name, f, ctx, true, false);
			} else {
				//dprintf("int value\n");
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
	const char* rv = "";
	if (def) {
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct ubasic_ctx* atomic = ubasic_clone(ctx);
		atomic->fn_type = RT_STRING;
		jump_linenum(def->line, atomic);

		while (!ubasic_finished(atomic)) {
			line_statement(atomic);
			if (atomic->errored) {
				ctx->errored = true;
				break;
			}
		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
			rv = (const char*)atomic->fn_return;
		}

		/* Only free the base struct! */
		kfree(atomic);

		free_local_heap(ctx);
		ctx->call_stack_ptr--;

		return rv;
	}
	tokenizer_error_print(ctx, "No such string FN");
	return rv;
}

int64_t ubasic_getproccount(struct ubasic_ctx* ctx)
{
	return proc_total();
}

int64_t ubasic_get_text_max_x(struct ubasic_ctx* ctx)
{
	return get_text_width();
}

int64_t ubasic_get_text_max_y(struct ubasic_ctx* ctx)
{
	return get_text_height();
}

int64_t ubasic_get_text_cur_x(struct ubasic_ctx* ctx)
{
	uint64_t x = 0, y = 0;
	get_text_position(&x, &y);
	return x;
}

int64_t ubasic_get_text_cur_y(struct ubasic_ctx* ctx)
{
	uint64_t x = 0, y = 0;
	get_text_position(&x, &y);
	return y;
}

int64_t ubasic_get_free_mem(struct ubasic_ctx* ctx)
{
	return get_free_memory();
}

int64_t ubasic_get_used_mem(struct ubasic_ctx* ctx)
{
	return get_used_memory();
}

int64_t ubasic_get_total_mem(struct ubasic_ctx* ctx)
{
	return get_total_memory();
}

int64_t ubasic_getprocid(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCID");
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
	PARAMS_END("RGB");
	return (uint32_t)(r << 16 | g << 8 | b);
}

char* ubasic_getprocname(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCNAME$");
	process_t* process = proc_find(proc_id(intval));
	return process && process->name ? gc_strdup(process->name) : "";
}

int64_t ubasic_getprocparent(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCPARENT");
	process_t* process = proc_find(proc_id(intval));
	return process ? process->ppid : 0;
}

int64_t ubasic_getproccpuid(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCCPUID");
	process_t* process = proc_find(proc_id(intval));
	return process ? process->cpu : 0;
}

char* ubasic_ramdisk_from_device(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("RAMDISK$");
	const char* rd = init_ramdisk_from_storage(strval);
	if (!rd) {
		return "";
	}
	return gc_strdup(rd);
}

char* ubasic_ramdisk_from_size(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t blocks = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t block_size = intval;
	PARAMS_END("RAMDISK");
	const char* rd = init_ramdisk(blocks, block_size);
	if (!rd) {
		return "";
	}
	return gc_strdup(rd);
}

char* ubasic_inkey(struct ubasic_ctx* ctx)
{
	const uint8_t key[2] = { kgetc((console*)ctx->cons), 0 };
	
	if (*key == 255) {
		// hlt stops busy waiting for INKEY$
		__asm__ volatile("hlt");
		return "";
	} else {
		return gc_strdup((const char*)key);
	}
}

char* ubasic_insocket(struct ubasic_ctx* ctx)
{
	uint8_t input[2] = { 0, 0 };
	
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t fd = intval;
	PARAMS_END("INSOCKET$");

	if (fd < 0) {
		tokenizer_error_print(ctx, "Invalid socket descriptor");
		return "";
	}

	int rv = recv(fd, input, 1, false, 0);

	if (rv > 0) {
		input[1] = 0;
		return gc_strdup((const char*)input);
	} else if (rv < 0) {
		tokenizer_error_print(ctx, socket_error(rv));
	} else {
		__asm__ volatile("hlt");
	}
	return "";
}

int64_t ubasic_sockstatus(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t fd = intval;
	PARAMS_END("SOCKSTATUS");

	if (fd < 0) {
		return 0;
	}

	return is_connected(fd);
}

int64_t ubasic_ctrlkey(struct ubasic_ctx* ctx)
{
	return ctrl_held();
}

int64_t ubasic_shiftkey(struct ubasic_ctx* ctx)
{
	return shift_held();
}

int64_t ubasic_altkey(struct ubasic_ctx* ctx)
{
	return alt_held();
}

int64_t ubasic_capslock(struct ubasic_ctx* ctx)
{
	return caps_lock_on();
}

char* ubasic_chr(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("CHR$");
	char res[2] = {(unsigned char)intval, 0};
	return gc_strdup(res);
}

int64_t ubasic_instr(struct ubasic_ctx* ctx)
{
	char* haystack;
	char* needle;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	haystack = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	needle = strval;
	PARAMS_END("INSTR");
	size_t n_len = strlen(needle);
	for (size_t i = 0; i < strlen(haystack) - n_len + 1; ++i) {
		if (!strncmp(haystack + i, needle, n_len)) {
			return i + 1;
		}
	}
	return 0;
}

char* ubasic_netinfo(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("NETINFO$");
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
	PARAMS_END("DNS$");
	char ip[16] = { 0 };
	uint32_t addr = dns_lookup_host(getdnsaddr(), strval, 2);
	get_ip_str(ip, (uint8_t*)&addr);
	return gc_strdup(ip);
}

char* ubasic_upper(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("UPPER$");
	char* modified = gc_strdup(strval);
	for (char* m = modified; *m; ++m) {
		*m = toupper(*m);
	}
	return modified;
}

char* ubasic_lower(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("LOWER$");
	char* modified = gc_strdup(strval);
	for (char* m = modified; *m; ++m) {
		*m = tolower(*m);
	}
	return modified;
}

char* ubasic_tokenize(struct ubasic_ctx* ctx)
{
	char* varname, *split;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_VARIABLE);
	varname = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	split = strval;
	PARAMS_END("TOKENIZE$");
	const char* current_value = ubasic_get_string_variable(varname, ctx);
	const char* old_value = current_value;
	size_t len = strlen(current_value);
	size_t split_len = strlen(split);
	size_t ofs = 0;
	while (*current_value) {
		if (ofs + split_len > len) {
			break;
		} else if (!strncmp(current_value, split, split_len)) {
			char return_value[MAX_STRINGLEN];
			char new_value[MAX_STRINGLEN];
			strlcpy(return_value, old_value, ofs + split_len);
			strlcpy(new_value, old_value + ofs + split_len, MAX_STRINGLEN);
			ubasic_set_string_variable(varname, new_value, ctx, false, false);
			return gc_strdup(return_value);
		}
		current_value++;
		ofs++;
	}
	char return_value[MAX_STRINGLEN];
	strlcpy(return_value, old_value, MAX_STRINGLEN);
	ubasic_set_string_variable(varname, "", ctx, false, false);
	return gc_strdup(return_value);
}


char* ubasic_left(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("LEFT$");
	int64_t len = strlen(strval);
	if (intval < 0) {
		intval = 0;
	}
	if (len == 0 || intval == 0) {
		return "";
	}
	if (intval > len) {
		intval = len;
	}
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
	PARAMS_END("MID$");
	int64_t len = strlen(strval);
	if (len == 0) {
		return "";
	}
	if (start > len) {
		start = len;
	}
	if (start < 0) {
		start = 0;
	}
	if (end < start) {
		end = start;
	}
	if (end > len) {
		end = len;
	}
	char* cut = gc_strdup(strval);
	*(cut + end) = 0;
	return cut + start;
}

int64_t ubasic_len(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("LEN");
	return strlen(strval);
}

int64_t ubasic_abs(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("ABS");
	return labs(intval);
}

void ubasic_sin(struct ubasic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END("SIN");
	*res = sin(doubleval);
}

void ubasic_cos(struct ubasic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END("COS");
	*res = cos(doubleval);
}

void ubasic_tan(struct ubasic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END("TAN");
	*res = tan(doubleval);
}

void ubasic_pow(struct ubasic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double base = doubleval;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END("POW");
	*res = pow(base, doubleval);
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
	if (def) {
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct ubasic_ctx* atomic = ubasic_clone(ctx);
		atomic->fn_type = RT_INT;
		dprintf("Function eval, jump to line %d\n", def->line);
		jump_linenum(def->line, atomic);

		while (!ubasic_finished(atomic)) {
			line_statement(atomic);
			if (atomic->errored) {
				ctx->errored = true;
				break;
			}

		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
			rv = (int64_t)atomic->fn_return;
		}

		/* Only free the base struct! */
		kfree(atomic);

		free_local_heap(ctx);
		ctx->call_stack_ptr--;

		return rv;
	}
	tokenizer_error_print(ctx, "No such integer FN");
	return 0;
}

void ubasic_eval_double_fn(const char* fn_name, struct ubasic_ctx* ctx, double* res)
{
	struct ub_proc_fn_def* def = ubasic_find_fn(fn_name + 2, ctx);
	if (def) {
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct ubasic_ctx* atomic = ubasic_clone(ctx);
		atomic->fn_type = RT_FLOAT;
		jump_linenum(def->line, atomic);

		while (!ubasic_finished(atomic)) {
			line_statement(atomic);
			if (atomic->errored) {
				ctx->errored = true;
				break;
			}
		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
			*res = *((double*)atomic->fn_return);
		}

		/* Only free the base struct! */
		kfree(atomic);

		free_local_heap(ctx);
		ctx->call_stack_ptr--;

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
	return (*varname == 'F' && *(varname + 1) == 'N' && !strchr(varname, '#') && !strchr(varname, '$'));
}

char varname_is_string_function(const char* varname) {
	return (*varname == 'F' && *(varname + 1) == 'N' && strchr(varname, '$') && !strchr(varname, '#'));
}

char varname_is_double_function(const char* varname) {
	return (*varname == 'F' && *(varname + 1) == 'N' && strchr(varname, '#') && !strchr(varname, '$'));
}

const char* ubasic_test_string_variable(const char* var, struct ubasic_ctx* ctx)
{
	struct ub_var_string* list[] = {
		ctx->local_string_variables[ctx->call_stack_ptr],
		ctx->str_variables
	};
	for (int j = 0; j < 2; j++)
	{
		struct ub_var_string* cur = list[j];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname))	{
				return cur->value;
			}
		}
	}
	return NULL;
}

const char* ubasic_get_string_variable(const char* var, struct ubasic_ctx* ctx)
{
	char* retv;
	int t = ubasic_builtin_str_fn(var, ctx, &retv);
	if (t)
		return retv;

	if (varname_is_string_function(var)) {
		const char* res = ubasic_eval_str_fn(var, ctx);
		return res;
	}

	if (varname_is_string_array_access(ctx, var)) {
		return ubasic_get_string_array_variable(var, arr_variable_index(ctx), ctx);
	}

	struct ub_var_string* list[ctx->call_stack_ptr + 1];
	int j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_string_variables[j];
	}
	list[0] = ctx->str_variables;
	for (j = ctx->call_stack_ptr; j >= 0; --j)
	{
		struct ub_var_string* cur = list[j];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname))	{
				return cur->value;
			}
		}
	}

	char err[1024];
	sprintf(err, "No such string variable '%s'", var);
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

	if (varname_is_int_array_access(ctx, var)) {
		return ubasic_get_int_array_variable(var, arr_variable_index(ctx), ctx);
	}

	struct ub_var_int* list[ctx->call_stack_ptr + 1];
	int j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_int_variables[j];
	}
	list[0] = ctx->int_variables;
	for (j = ctx->call_stack_ptr; j >= 0; --j)
	{
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


	char err[MAX_STRINGLEN];
	snprintf(err, MAX_STRINGLEN, "No such integer variable '%s'", var);
	tokenizer_error_print(ctx, err);
	return 0; /* No such variable */
}

bool ubasic_get_double_variable(const char* var, struct ubasic_ctx* ctx, double* res)
{
	if (ubasic_builtin_double_fn(var, ctx, res)) {
		return true;
	}
		
	if (varname_is_double_function(var)) {
		ubasic_eval_double_fn(var, ctx, res);
		return true;
	}

	if (varname_is_double_array_access(ctx, var)) {
		return ubasic_get_double_array_variable(var, arr_variable_index(ctx), ctx, res);
	}


	struct ub_var_double* list[ctx->call_stack_ptr + 1];
	int j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_double_variables[j];
	}
	list[0] = ctx->double_variables;
	for (j = ctx->call_stack_ptr; j >= 0; --j)
	{
		struct ub_var_double* cur = list[j];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname))	{
				*res = cur->value;
				return true;
			}
		}
	}


	char err[1024];
	if (var[strlen(var) - 1] == '#') {
		sprintf(err, "No such real variable '%s'", var);
		tokenizer_error_print(ctx, err);
	}
	*res = 0.0; /* No such variable */
	return false;
}

ub_return_type ubasic_get_numeric_variable(const char* var, struct ubasic_ctx* ctx, double* res)
{
	if (ubasic_get_double_variable(var, ctx, res)) {
		return RT_INT;
	}
	*res = (double)(ubasic_get_int_variable(var, ctx));
	return RT_FLOAT;
}

int64_t ubasic_get_numeric_int_variable(const char* var, struct ubasic_ctx* ctx)
{
	double res;
	if (ubasic_get_double_variable(var, ctx, &res)) {
		return (int64_t)res;
	}
	return ubasic_get_int_variable(var, ctx);
}
