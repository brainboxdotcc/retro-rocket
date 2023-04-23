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
	{ ubasic_readstring, "READ$" },
	{ ubasic_getname, "GETNAME$" },
	{ ubasic_getprocname, "GETPROCNAME$" },
	{ ubasic_ramdisk_from_device, "RAMDISK$" },
	{ ubasic_ramdisk_from_size, "RAMDISK" },
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
	size_t new_size_max = strlen(program) * 2;
	char* newprog = kmalloc(new_size_max);
	char line_buffer[MAX_STRINGLEN];
	char* line_ptr = line_buffer;
	bool insert_line = true;
	*newprog = 0;
	while (*program) {
		if (insert_line) {
			while (*program == '\n') {
				*line_ptr++ = '\n';
				program++;
			}
			snprintf(line_buffer, MAX_STRINGLEN, "%d ", line);
			line += increment;
			insert_line = false;
			line_ptr = line_buffer + strlen(line_buffer);
		}
		if (*program == '\n') {
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

	ctx->errored = false;
	ctx->current_token = ERROR;
	ctx->int_variables = NULL;
	ctx->str_variables = NULL;
	ctx->double_variables = NULL;
	ctx->int_array_variables = NULL;
	ctx->string_array_variables = NULL;
	ctx->double_array_variables = NULL;
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
	if (ctx->program_ptr == NULL) {
		kfree(ctx);
		*error = "Out of memory";
		return NULL;
	}
	ctx->lines = hashmap_new(sizeof(ub_line_ref), 0, 5923530135432, 458397058, line_hash, line_compare, NULL, NULL);

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

	/* Build doubly linked list of line number references */
	uint32_t last_line = 0xFFFFFFFF;
	while (!tokenizer_finished(ctx)) {
		uint32_t line = tokenizer_num(ctx, NUMBER);
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
	struct ubasic_ctx* ctx = (struct ubasic_ctx*)kmalloc(sizeof(struct ubasic_ctx));
	int i;

	ctx->current_token = ERROR;
	ctx->int_variables = old->int_variables;
	ctx->str_variables = old->str_variables;
	ctx->double_variables = old->double_variables;
	ctx->int_array_variables = old->int_array_variables;
	ctx->string_array_variables = old->string_array_variables;
	ctx->double_array_variables = old->double_array_variables;
	ctx->lines = old->lines;

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
			
			char* linetext = (char*)kmalloc(lineend - linestart + 1);
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
							struct ub_param* par = (struct ub_param*)kmalloc(sizeof(struct ub_param));

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
	for (size_t x = 0; x < ctx->gosub_stack_ptr; x++) {
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
		tokenizer_error_print(ctx, "No such line");
		return false;
	}
	ctx->ptr = line->ptr;
	ctx->current_token = get_next_token(ctx);
	return true;
}

static void goto_statement(struct ubasic_ctx* ctx)
{
	accept(GOTO, ctx);
	jump_linenum(tokenizer_num(ctx, NUMBER), ctx);
}

static void colour_statement(struct ubasic_ctx* ctx, int tok)
{
	accept(tok, ctx);
	setforeground((console*)ctx->cons, expr(ctx));
	accept(NEWLINE, ctx);
}

static void background_statement(struct ubasic_ctx* ctx)
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
				sprintf(buffer, "%s", str_expr(ctx));
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

static void print_statement(struct ubasic_ctx* ctx)
{
	accept(PRINT, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		kprintf(out);
	}
}

static void dim_statement(struct ubasic_ctx* ctx)
{
	accept(DIM, ctx);
	const char* array_name = tokenizer_variable_name(ctx);
	accept(VARIABLE, ctx);
	accept(COMMA, ctx);
	int64_t array_size = expr(ctx);
	accept(NEWLINE, ctx);
	char last = array_name[strlen(array_name) - 1];
	switch (last) {
		case '#':
			ubasic_dim_double_array(array_name, array_size, ctx);
		break;
		case '$':
			ubasic_dim_string_array(array_name, array_size, ctx);
		break;
		default:
			ubasic_dim_int_array(array_name, array_size, ctx);
	}
}

static void redim_statement(struct ubasic_ctx* ctx)
{
	accept(REDIM, ctx);
	const char* array_name = tokenizer_variable_name(ctx);
	accept(VARIABLE, ctx);
	accept(COMMA, ctx);
	int64_t array_size = expr(ctx);
	accept(NEWLINE, ctx);
	char last = array_name[strlen(array_name) - 1];
	switch (last) {
		case '#':
			ubasic_redim_double_array(array_name, array_size, ctx);
		break;
		case '$':
			ubasic_redim_string_array(array_name, array_size, ctx);
		break;
		default:
			ubasic_redim_int_array(array_name, array_size, ctx);
	}
}

static void sockwrite_statement(struct ubasic_ctx* ctx)
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

static void mkdir_statement(struct ubasic_ctx* ctx)
{
	accept(MKDIR, ctx);
	const char* name = str_expr(ctx);
	accept(NEWLINE, ctx);
	if (!fs_create_directory(name)) {
		tokenizer_error_print(ctx, "Unable to create directory");
	}
}

static void mount_statement(struct ubasic_ctx* ctx)
{
	accept(MOUNT, ctx);
	const char* path = str_expr(ctx);
	accept(COMMA, ctx);
	const char* device = str_expr(ctx);
	accept(COMMA, ctx);
	const char* fs_type = str_expr(ctx);
	accept(NEWLINE, ctx);
	filesystem_mount(path, device, fs_type);
}

static void rmdir_statement(struct ubasic_ctx* ctx)
{
	accept(RMDIR, ctx);
	const char* name = str_expr(ctx);
	accept(NEWLINE, ctx);
	if (!fs_delete_directory(name)) {
		tokenizer_error_print(ctx, "Unable to delete directory");
	}
}


static void delete_statement(struct ubasic_ctx* ctx)
{
	accept(DELETE, ctx);
	const char* name = str_expr(ctx);
	accept(NEWLINE, ctx);
	if (!fs_delete_file(name)) {
		tokenizer_error_print(ctx, "Unable to delete file");
	}
}


static void write_statement(struct ubasic_ctx* ctx)
{
	int fd = -1;

	accept(WRITE, ctx);
	fd = ubasic_get_numeric_int_variable(tokenizer_variable_name(ctx), ctx);
	accept(VARIABLE, ctx);
	accept(COMMA, ctx);
	char* out = printable_syntax(ctx);
	if (out) {
		_write(fd, out, strlen(out));
	}
}

static void proc_statement(struct ubasic_ctx* ctx)
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
		ctx->gosub_stack_ptr++;

		if (*ctx->ptr == '(' && *(ctx->ptr + 1) != ')') {
			begin_comma_list(def, ctx);
			while (extract_comma_list(def, ctx));
		}
		struct ubasic_ctx* atomic = ubasic_clone(ctx);
		atomic->fn_type = RT_NONE;
		jump_linenum(def->line, atomic);

		while (!ubasic_finished(atomic)) {
			line_statement(atomic);
		}
		/* Only free the base struct! */
		kfree(atomic);

		ctx->gosub_stack_ptr--;
		while (tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT) {
			tokenizer_next(ctx);
		}
		accept(NEWLINE, ctx);
		return;
	}
	tokenizer_error_print(ctx, "No such PROC");
}

bool conditional(struct ubasic_ctx* ctx)
{
	char current_line[MAX_STRINGLEN];
	char* pos = strchr(ctx->ptr, '\n');
	char* end = strchr(ctx->ptr, 0);
	bool stringlike = false, real = false;
	strlcpy(current_line, ctx->ptr, pos ? pos - ctx->ptr + 1 : end - ctx->ptr + 1);
	if (strlen(current_line) > 10) { // "IF 1 THEN ..."
		for (char* n = current_line; *n && *n != '\n'; ++n) {
			if (isalnum(*n) && *(n + 1) == '$') {
				stringlike = true; /* String variable */
				break;
			} else if (isalnum(*n) && *(n + 1) == '#') {
				real = true; /* Real variable */
				break;
			} else if (isdigit(*n) && *(n + 1) == '.' && isdigit(*(n + 2))) {
				real = true; /* Decimal number */
				break;
			} else if (*n == ' ' && *(n + 1) == 'T' && *(n + 2) == 'H' && *(n + 3) == 'E' && *(n + 4) == 'N') {
				break;
			} else if (*n == '\n') {
				break;
			}
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

static void if_statement(struct ubasic_ctx* ctx)
{
	accept(IF, ctx);
	bool r = conditional(ctx);
	accept(THEN, ctx);
	if (r) {
		statement(ctx);
	} else {
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

static void chain_statement(struct ubasic_ctx* ctx)
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

static void eval_statement(struct ubasic_ctx* ctx)
{
	accept(EVAL, ctx);
	const char* v = str_expr(ctx);
	accept(NEWLINE, ctx);

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
		/* If program doesn't end in newline, add one */
		char last = ctx->program_ptr[strlen(ctx->program_ptr) - 1];
		if (last > 13) {
			strlcat(ctx->program_ptr, "\n", ctx->oldlen + 5000);
		}
		const char* line_9998 = (ctx->program_ptr + strlen(ctx->program_ptr));
		strlcat(ctx->program_ptr, "9998 ", ctx->oldlen + 5000);
		strlcat(ctx->program_ptr, clean_v, ctx->oldlen + 5000);
		const char* line_9999 = (ctx->program_ptr + strlen(ctx->program_ptr) + 1);
		strlcat(ctx->program_ptr, "\n9999 RETURN\n", ctx->oldlen + 5000);

		hashmap_set(ctx->lines, &(ub_line_ref){ .line_number = 9998, .ptr = line_9998 });
		hashmap_set(ctx->lines, &(ub_line_ref){ .line_number = 9999, .ptr = line_9999 });

		ctx->eval_linenum = ctx->current_linenum;
		ctx->gosub_stack[ctx->gosub_stack_ptr++] = ctx->current_linenum;

		jump_linenum(9998, ctx);
	} else {
		ctx->program_ptr[ctx->oldlen] = 0;
		ctx->oldlen = 0;
		ctx->eval_linenum = 0;
		/* Delete references to the eval lines */
		hashmap_delete(ctx->lines, &(ub_line_ref){ .line_number = 9998 });
		hashmap_delete(ctx->lines, &(ub_line_ref){ .line_number = 9999 });
	}
}

static void rem_statement(struct ubasic_ctx* ctx)
{
	accept(REM, ctx);
	while (tokenizer_token(ctx) != ENDOFINPUT && tokenizer_token(ctx) != NEWLINE) {
		tokenizer_next(ctx);
	}
	accept(NEWLINE, ctx);
}

static void def_statement(struct ubasic_ctx* ctx)
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

static void openin_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "OPENIN is a function");
}

static void openup_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "OPENUP is a function");
}

static void openout_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "OPENOUT is a function");
}

static void read_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "READ is a function");
}

static void close_statement(struct ubasic_ctx* ctx)
{
	accept(CLOSE, ctx);
	_close(expr(ctx));
	accept(NEWLINE, ctx);
}

static void eof_statement(struct ubasic_ctx* ctx)
{
	tokenizer_error_print(ctx, "EOF is a function");
}

static void input_statement(struct ubasic_ctx* ctx)
{
	accept(INPUT, ctx);
	const char* var = tokenizer_variable_name(ctx);
	accept(VARIABLE, ctx);

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

static void sockread_statement(struct ubasic_ctx* ctx)
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

static void connect_statement(struct ubasic_ctx* ctx)
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

static void sockclose_statement(struct ubasic_ctx* ctx)
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

static int64_t arr_expr_set_index(struct ubasic_ctx* ctx, const char* varname)
{
	while(*ctx->ptr != '(' && *ctx->ptr != '\n' && *ctx->ptr) ctx->ptr++;
	if (*ctx->ptr != '(') {
		accept(VARIABLE, ctx);	
		accept(EQUALS, ctx);
		return -1;
	}
	ctx->ptr++;
	ctx->current_token = get_next_token(ctx);
	if (*ctx->ptr == '-') {
		tokenizer_error_print(ctx, "Array index out of bounds");
		return 0;
	}
	int64_t index = expr(ctx);
	accept(CLOSEBRACKET, ctx);
	accept(EQUALS, ctx);
	return index;
}

static void let_statement(struct ubasic_ctx* ctx, bool global)
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
			ubasic_set_string_variable(var, _expr, ctx, false, global);
		break;
		case '#':
			double_expr(ctx, &f_expr);
			ubasic_set_double_variable(var, f_expr, ctx, false, global);
		break;
		default:
			ubasic_set_int_variable(var, expr(ctx), ctx, false, global);
		break;
	}
	accept(NEWLINE, ctx);
}

static void cls_statement(struct ubasic_ctx* ctx)
{
	accept(CLS, ctx);
	clearscreen(current_console);
	accept(NEWLINE, ctx);
}

static void gcol_statement(struct ubasic_ctx* ctx)
{
	accept(GCOL, ctx);
	ctx->graphics_colour = expr(ctx);
	//dprintf("New graphics color: %08X\n", ctx->graphics_colour);
	accept(NEWLINE, ctx);
}

static void gotoxy_statement(struct ubasic_ctx* ctx)
{
	accept(CURSOR, ctx);
	int64_t x = expr(ctx);
	accept(COMMA, ctx);
	int64_t y = expr(ctx);
	gotoxy(x, y);
	accept(NEWLINE, ctx);
}

static void draw_line_statement(struct ubasic_ctx* ctx)
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

static void point_statement(struct ubasic_ctx* ctx)
{
	accept(POINT, ctx);
	int64_t x1 = expr(ctx);
	accept(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept(NEWLINE, ctx);
	putpixel(x1, y1, ctx->graphics_colour);
}

static void triangle_statement(struct ubasic_ctx* ctx)
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

static void rectangle_statement(struct ubasic_ctx* ctx)
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

static void circle_statement(struct ubasic_ctx* ctx)
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

static void gosub_statement(struct ubasic_ctx* ctx)
{
	int linenum;

	accept(GOSUB, ctx);
	linenum = tokenizer_num(ctx, NUMBER);
	accept(NUMBER, ctx);
	accept(NEWLINE, ctx);

	if (ctx->gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
		ctx->gosub_stack[ctx->gosub_stack_ptr] = tokenizer_num(ctx, NUMBER);
		ctx->gosub_stack_ptr++;
		jump_linenum(linenum, ctx);
	} else {
		tokenizer_error_print(ctx, "gosub_statement: gosub stack exhausted");
	}
}

static void return_statement(struct ubasic_ctx* ctx)
{
	accept(RETURN, ctx);
	if (ctx->gosub_stack_ptr > 0) {
		ctx->gosub_stack_ptr--;
		jump_linenum(ctx->gosub_stack[ctx->gosub_stack_ptr], ctx);
	} else {
		tokenizer_error_print(ctx, "return_statement: non-matching return");
	}
}

static void next_statement(struct ubasic_ctx* ctx)
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

static void for_statement(struct ubasic_ctx* ctx)
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

	if (ctx->for_stack_ptr < MAX_FOR_STACK_DEPTH) {
		ctx->for_stack[ctx->for_stack_ptr].line_after_for = tokenizer_num(ctx, NUMBER);
		ctx->for_stack[ctx->for_stack_ptr].for_variable = for_variable;
		ctx->for_stack[ctx->for_stack_ptr].to = to;
		ctx->for_stack[ctx->for_stack_ptr].step = step;
		ctx->for_stack_ptr++;
	} else {
		tokenizer_error_print(ctx, "Too many FOR");
	}
}

static void repeat_statement(struct ubasic_ctx* ctx)
{
	accept(REPEAT, ctx);
	accept(NEWLINE, ctx);
	if (ctx->gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
		ctx->gosub_stack[ctx->gosub_stack_ptr] = tokenizer_num(ctx, NUMBER);
		ctx->gosub_stack_ptr++;
	} else {
		tokenizer_error_print(ctx, "REPEAT stack exhausted");
	}

}


static void until_statement(struct ubasic_ctx* ctx)
{
	accept(UNTIL, ctx);
	bool done = conditional(ctx);
	accept(NEWLINE, ctx);

	if (ctx->gosub_stack_ptr > 0) {
		if (!done) {
			jump_linenum(ctx->gosub_stack[ctx->gosub_stack_ptr - 1], ctx);
		} else {
			ctx->gosub_stack_ptr--;
		}
	} else {
		tokenizer_error_print(ctx, "UNTIL without REPEAT");
	}

}



static void end_statement(struct ubasic_ctx* ctx)
{
	accept(END, ctx);
	ctx->ended = true;
}

static void eq_statement(struct ubasic_ctx* ctx)
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

static void retproc_statement(struct ubasic_ctx* ctx)
{
	accept(RETPROC, ctx);
	accept(NEWLINE, ctx);
	if (ctx->fn_type != RT_NONE)  {
		tokenizer_error_print(ctx, "Can't RETPROC from a FN");
		return;
	}
	ctx->ended = true;
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
		case CIRCLE:
			return circle_statement(ctx);
		case LET:
			accept(LET, ctx);
			/* Fall through. */
		case VARIABLE:
			return let_statement(ctx, false);
		case GLOBAL:
			accept(GLOBAL, ctx);
			return let_statement(ctx, true);
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
	if (ctx->errored) {
		ctx->errored = false;
		if (ctx->gosub_stack_ptr > 0) {
			ctx->gosub_stack_ptr--;
			if (jump_linenum(ctx->gosub_stack[ctx->gosub_stack_ptr], ctx)) {
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
	bool error_set = false;
	struct ub_var_string* list[] = {
		ctx->str_variables,
		ctx->local_string_variables[ctx->gosub_stack_ptr]
	};

	if (*value && !strcmp(var, "ERROR$")) {
		error_set = true;
	}

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

const char* ubasic_get_string_array_variable(const char* var, int64_t index, struct ubasic_ctx* ctx)
{
	if (index < 0) {
		tokenizer_error_print(ctx, "Array index out of bounds");
		return "";
	}
	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_print(ctx, "Array index out of bounds");
				return "";
			}
			if (cur->values[index]) {
				return gc_strdup(cur->values[index]);
			}
			return "";
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return "";
}

int64_t ubasic_get_int_array_variable(const char* var, int64_t index, struct ubasic_ctx* ctx)
{
	if (index < 0) {
		tokenizer_error_print(ctx, "Array index out of bounds");
		return 0;
	}
	dprintf("Get array var: '%s' index %d\n", var, index);
	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_print(ctx, "Array index out of bounds");
				return 0;
			}
			return cur->values[index];
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return 0;
}

bool ubasic_get_double_array_variable(const char* var, int64_t index, struct ubasic_ctx* ctx, double* ret)
{
	if (index < 0) {
		tokenizer_error_print(ctx, "Array index out of bounds");
		*ret = 0;
		return false;
	}
	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_print(ctx, "Array index out of bounds");
				return false;
			}
			*ret = cur->values[index];
			return true;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	*ret = 0;
	return false;
}

bool ubasic_dim_int_array(const char* var, int64_t size, struct ubasic_ctx* ctx)
{
	if (!valid_int_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return false;
	}
	if (size < 1) {
		tokenizer_error_print(ctx, "Invalid array size");
		return false;
	}

	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			tokenizer_error_print(ctx, "Array already dimensioned");
			return false;
		}
	}
	struct ub_var_int_array* new = kmalloc(sizeof(ub_var_int_array));
	new->itemcount = size;
	new->next = ctx->int_array_variables;
	new->varname = strdup(var);
	new->values = kmalloc(sizeof(int64_t) * size);
	for (int64_t v = 0; v < size; ++v) {
		new->values[v] = 0;
	}
	ctx->int_array_variables = new;
	return true;
}

bool ubasic_dim_string_array(const char* var, int64_t size, struct ubasic_ctx* ctx)
{
	if (!valid_string_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return false;
	}
	if (size < 1) {
		tokenizer_error_print(ctx, "Invalid array size");
		return false;
	}

	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			tokenizer_error_print(ctx, "Array already dimensioned");
			return false;
		}
	}
	struct ub_var_string_array* new = kmalloc(sizeof(ub_var_string_array));
	new->itemcount = size;
	new->next = ctx->string_array_variables;
	new->varname = strdup(var);
	new->values = kmalloc(sizeof(char*) * size);
	for (int64_t v = 0; v < size; ++v) {
		new->values[v] = NULL;
	}
	ctx->string_array_variables = new;
	return true;	
}

bool ubasic_dim_double_array(const char* var, int64_t size, struct ubasic_ctx* ctx)
{
	if (!valid_double_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return false;
	}
	if (size < 1) {
		tokenizer_error_print(ctx, "Invalid array size");
		return false;
	}

	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			tokenizer_error_print(ctx, "Array already dimensioned");
			return false;
		}
	}
	struct ub_var_double_array* new = kmalloc(sizeof(ub_var_double_array));
	new->itemcount = size;
	new->next = ctx->double_array_variables;
	new->varname = strdup(var);
	new->values = kmalloc(sizeof(double) * size);
	for (int64_t v = 0; v < size; ++v) {
		new->values[v] = 0.0;
	}
	ctx->double_array_variables = new;
	return true;		
}

bool ubasic_redim_int_array(const char* var, int64_t size, struct ubasic_ctx* ctx)
{
	if (size < 1) {
		tokenizer_error_print(ctx, "Invalid array size");
		return false;
	}
	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)size == cur->itemcount) {
				return true;
			}
			cur->values = krealloc(cur->values, sizeof(int64_t) * size);
			if ((uint64_t)size > cur->itemcount) {
				/* If array is being expanded, zero the new entries */
				for (int64_t i = cur->itemcount; i < size; ++i) {
					cur->values[i] = 0;
				}
			}
			cur->itemcount = size;
			return true;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return false;
}

bool ubasic_redim_string_array(const char* var, int64_t size, struct ubasic_ctx* ctx)
{
	if (size < 1) {
		tokenizer_error_print(ctx, "Invalid array size");
		return false;
	}
	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)size == cur->itemcount) {
				return true;
			}
			if ((uint64_t)size < cur->itemcount) {
				/* If string array is being reduced in size, free strings that fall in the freed area */
				for (uint64_t x = size; x < (uint64_t)cur->itemcount; ++x) {
					if (cur->values[x]) {
						kfree(cur->values[x]);
					}
				}
			}
			cur->values = krealloc(cur->values, sizeof(char*) * size);
			if ((uint64_t)size > cur->itemcount) {
				/* If array is being expanded, zero the new entries */
				for (int64_t i = cur->itemcount; i < size; ++i) {
					cur->values[i] = NULL;
				}
			}
			cur->itemcount = size;
			return true;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return false;	
}

bool ubasic_redim_double_array(const char* var, int64_t size, struct ubasic_ctx* ctx)
{
	if (size < 1) {
		tokenizer_error_print(ctx, "Invalid array size");
		return false;
	}
	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)size == cur->itemcount) {
				return true;
			}
			cur->values = krealloc(cur->values, sizeof(double) * size);
			if ((uint64_t)size > cur->itemcount) {
				/* If array is being expanded, zero the new entries */
				for (int64_t i = cur->itemcount; i < size; ++i) {
					cur->values[i] = 0;
				}
			}
			cur->itemcount = size;
			return true;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return false;	
}

void ubasic_set_string_array_variable(const char* var, int64_t index, const char* value, struct ubasic_ctx* ctx)
{
	if (!valid_string_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}
	if (index < 0) {
		tokenizer_error_print(ctx, "Array index out of bounds");
		return;
	}

	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_print(ctx, "Array index out of bounds");
				return;
			}
			if (cur->values[index]) {
				kfree(cur->values[index]);
			}
			cur->values[index] = strdup(value);
			return;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
}

void ubasic_set_string_array(const char* var, const char* value, struct ubasic_ctx* ctx)
{
	if (!valid_string_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}
	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			for (uint64_t x = 0; x < cur->itemcount; ++x) {
				if (cur->values[x]) {
					kfree(cur->values[x]);
				}
				cur->values[x] = strdup(value);
			}
		}
	}
}

void ubasic_set_int_array(const char* var, int64_t value, struct ubasic_ctx* ctx)
{
	if (!valid_int_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}
	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			for (uint64_t x = 0; x < cur->itemcount; ++x) {
				cur->values[x] = value;
			}
		}
	}
}

void ubasic_set_double_array(const char* var, double value, struct ubasic_ctx* ctx)
{
	if (!valid_double_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}
	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			for (uint64_t x = 0; x < cur->itemcount; ++x) {
				cur->values[x] = value;
			}
		}
	}
}

void ubasic_set_double_array_variable(const char* var, int64_t index, double value, struct ubasic_ctx* ctx)
{
	if (!valid_double_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}
	if (index < 0) {
		tokenizer_error_print(ctx, "Array index out of bounds");
		return;
	}

	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_print(ctx, "Array index out of bounds");
				return;
			}
			cur->values[index] = value;
			return;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
}

void ubasic_set_int_array_variable(const char* var, int64_t index, int64_t value, struct ubasic_ctx* ctx)
{
	if (!valid_int_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}
	if (index < 0) {
		tokenizer_error_print(ctx, "Array index out of bounds");
		return;
	}

	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_print(ctx, "Array index out of bounds");
				return;
			}
			cur->values[index] = value;
			return;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
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
			//dprintf("Set int variable '%s' to '%d' (gosub local)\n", var, value);
			ctx->local_int_variables[ctx->gosub_stack_ptr] = kmalloc(sizeof(struct ub_var_int));
			ctx->local_int_variables[ctx->gosub_stack_ptr]->next = NULL;
			ctx->local_int_variables[ctx->gosub_stack_ptr]->varname = strdup(var);
			ctx->local_int_variables[ctx->gosub_stack_ptr]->value = value;
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
			cur = ctx->local_int_variables[ctx->gosub_stack_ptr];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname)) {
				//dprintf("Set int variable '%s' to '%d' (updating)\n", var, value);
				cur->value = value;
				return;
			}
		}
		//dprintf("Set int variable '%s' to '%d'\n", var, value);
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
	//char buffer[MAX_STRINGLEN];

	if (!valid_double_var(var)) {
		tokenizer_error_print(ctx, "Malformed variable name");
		return;
	}

	if (list[local] == NULL) {
		if (local) {
			//dprintf("Set double variable '%s' to '%s' (gosub local)\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
			ctx->local_double_variables[ctx->gosub_stack_ptr] = kmalloc(sizeof(struct ub_var_double));
			ctx->local_double_variables[ctx->gosub_stack_ptr]->next = NULL;
			ctx->local_double_variables[ctx->gosub_stack_ptr]->varname = strdup(var);
			ctx->local_double_variables[ctx->gosub_stack_ptr]->value = value;
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
			cur = ctx->local_double_variables[ctx->gosub_stack_ptr];
		for (; cur; cur = cur->next) {
			if (!strcmp(var, cur->varname)) {
				//dprintf("Set double variable '%s' to '%s' (updating)\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
				cur->value = value;
				return;
			}
		}
		//dprintf("Set double variable '%s' to '%s'\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
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
	if (def) {
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
#define BIP_DOUBLE 2

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

#define PARAMS_END(NAME) { \
	ctx->ptr--; \
	if (*ctx->ptr != ')') \
		tokenizer_error_print(ctx, "Too many parameters for function " NAME); \
}

int64_t ubasic_open_func(struct ubasic_ctx* ctx, int oflag)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	if (fs_is_directory(strval)) {
		tokenizer_error_print(ctx, "Not a file");
		return 0;
	}
	int fd = _open(strval, oflag);
	dprintf("ubasic_open_func: %s returned: %d\n", strval, fd);
	return fd;
}

int64_t ubasic_openin(struct ubasic_ctx* ctx)
{
	return ubasic_open_func(ctx, _O_RDONLY);
}

int64_t ubasic_openout(struct ubasic_ctx* ctx)
{
	return ubasic_open_func(ctx, _O_WRONLY);
}

int64_t ubasic_openup(struct ubasic_ctx* ctx)
{
	return ubasic_open_func(ctx, _O_RDWR);
}

int64_t ubasic_getnamecount(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	if (!fs_is_directory(strval)) {
		tokenizer_error_print(ctx, "Not a directory");
		return 0;
	}
	fs_directory_entry_t* fsl = fs_get_items(strval);
	int count = 0;
	while (fsl) {
		fsl = fsl->next;
		count++;
	}
	return count;
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
	process_t* process = proc_find(proc_id(intval));
	return process && process->name ? gc_strdup(process->name) : "";
}

int64_t ubasic_getprocparent(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	process_t* process = proc_find(proc_id(intval));
	return process ? process->ppid : 0;
}

int64_t ubasic_getproccpuid(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	process_t* process = proc_find(proc_id(intval));
	return process ? process->cpu : 0;
}

char* ubasic_ramdisk_from_device(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* rd = init_ramdisk_from_storage(strval);
	if (!rd) {
		return gc_strdup("");
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
	const char* rd = init_ramdisk(blocks, block_size);
	if (!rd) {
		return gc_strdup("");
	}
	return gc_strdup(rd);
}

char* ubasic_getname(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	if (!fs_is_directory(strval)) {
		tokenizer_error_print(ctx, "Not a directory");
		return 0;
	}
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
	return labs(intval);
}

void ubasic_sin(struct ubasic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	*res = sin(doubleval);
}

void ubasic_cos(struct ubasic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	*res = cos(doubleval);
}

void ubasic_tan(struct ubasic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	*res = tan(doubleval);
}

void ubasic_pow(struct ubasic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double base = doubleval;
	PARAMS_GET_ITEM(BIP_DOUBLE);
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
		ctx->gosub_stack_ptr++;

		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct ubasic_ctx* atomic = ubasic_clone(ctx);
		atomic->fn_type = RT_INT;
		dprintf("Function eval, jump to line %d\n", def->line);
		jump_linenum(def->line, atomic);

		while (!ubasic_finished(atomic)) {
			line_statement(atomic);
		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
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
	if (def) {
		ctx->gosub_stack_ptr++;

		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct ubasic_ctx* atomic = ubasic_clone(ctx);
		atomic->fn_type = RT_FLOAT;
		jump_linenum(def->line, atomic);

		while (!ubasic_finished(atomic)) {
			line_statement(atomic);
		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
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
		ctx->local_string_variables[ctx->gosub_stack_ptr],
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

bool varname_is_int_array_access(struct ubasic_ctx* ctx, const char* varname)
{
	for (ub_var_int_array* i = ctx->int_array_variables; i; i = i->next) {
		if (!strcmp(i->varname, varname)) {
			return true;
		}
	}
	return false;
}

bool varname_is_string_array_access(struct ubasic_ctx* ctx, const char* varname)
{
	for (ub_var_string_array* i = ctx->string_array_variables; i; i = i->next) {
		if (!strcmp(i->varname, varname)) {
			return true;
		}
	}
	return false;
}

bool varname_is_double_array_access(struct ubasic_ctx* ctx, const char* varname)
{
	for (ub_var_double_array* i = ctx->double_array_variables; i; i = i->next) {
		if (!strcmp(i->varname, varname)) {
			return true;
		}
	}
	return false;
}

int64_t arr_variable_index(struct ubasic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	return intval;
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

	if (varname_is_int_array_access(ctx, var)) {
		return ubasic_get_int_array_variable(var, arr_variable_index(ctx), ctx);
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


	char err[MAX_STRINGLEN];
	snprintf(err, MAX_STRINGLEN, "No such variable '%s'", var);
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
				return true;
			}
		}
	}


	char err[1024];
	if (var[strlen(var) - 1] == '#') {
		sprintf(err, "No such REAL variable '%s'", var);
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
