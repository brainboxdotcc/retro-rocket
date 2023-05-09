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
#include <cpuid.h>

struct basic_int_fn builtin_int[] =
{
	{ basic_abs, "ABS" },
	{ basic_len, "LEN" },
	{ basic_openin, "OPENIN" },
	{ basic_openout, "OPENOUT" },
	{ basic_openup, "OPENUP" },
	{ basic_eof, "EOF" },
	{ basic_read, "READ" },
	{ basic_instr, "INSTR" },
	{ basic_asc, "ASC" },
	{ basic_getnamecount, "GETNAMECOUNT" },
	{ basic_getsize, "GETSIZE" },
	{ basic_getproccount, "GETPROCCOUNT" },
	{ basic_getprocid, "GETPROCID" },
	{ basic_getprocparent, "GETPROCPARENT" },
	{ basic_getproccpuid, "GETPROCCPUID" },
	{ basic_rgb, "RGB" },
	{ basic_get_text_max_x, "TERMWIDTH" },
	{ basic_get_text_max_y, "TERMHEIGHT" },
	{ basic_get_text_cur_x, "CURRENTX" },
	{ basic_get_text_cur_y, "CURRENTY" },
	{ basic_get_free_mem, "MEMFREE" },
	{ basic_get_used_mem, "MEMUSED" },
	{ basic_get_total_mem, "MEMORY" },
	{ basic_sockstatus, "SOCKSTATUS" },
	{ basic_ctrlkey, "CTRLKEY" },
	{ basic_shiftkey, "SHIFTKEY" },
	{ basic_altkey, "ALTKEY" },
	{ basic_capslock, "CAPSLOCK" },
	{ basic_random, "RND" },
	{ basic_val, "VAL" },
	{ basic_hexval, "HEXVAL" },
	{ basic_octval, "OCTVAL" },
	{ basic_legacy_cpuid, "LCPUID" },
	{ basic_legacy_getlastcpuid, "LGETLASTCPUID" },
	{ basic_cpuid, "CPUID" },
	{ NULL, NULL }
};

struct basic_double_fn builtin_double[] = {
	{ basic_sin, "SIN" },
	{ basic_cos, "COS" },
	{ basic_tan, "TAN" },
	{ basic_pow, "POW" },
	{ basic_realval, "REALVAL" },
	{ basic_sqrt, "SQRT"},
	{ NULL, NULL },
};

struct basic_str_fn builtin_str[] =
{
	{ basic_netinfo, "NETINFO$" },
	{ basic_dns, "DNS$" },
	{ basic_left, "LEFT$" },
	{ basic_right, "RIGHT$" },
	{ basic_mid, "MID$" },
	{ basic_chr, "CHR$" },
	{ basic_insocket, "INSOCKET$" },
	{ basic_readstring, "READ$" },
	{ basic_getname, "GETNAME$" },
	{ basic_getprocname, "GETPROCNAME$" },
	{ basic_ramdisk_from_device, "RAMDISK$" },
	{ basic_ramdisk_from_size, "RAMDISK" },
	{ basic_inkey, "INKEY$" },
	{ basic_upper, "UPPER$" },
	{ basic_lower, "LOWER$" },
	{ basic_tokenize, "TOKENIZE$" },
	{ basic_csd, "CSD$" },
	{ basic_filetype, "FILETYPE$" },
	{ basic_str, "STR$" },
	{ basic_bool, "BOOL$" },
	{ basic_cpugetbrand, "CPUGETBRAND$" },
	{ basic_cpugetvendor, "CPUGETVENDOR$" },
	{ basic_intoasc, "INTOASC$" },
	{ NULL, NULL }
};

const struct g_cpuid_vendor cpuid_vendors[] =
{
	{ "VENDORAMDK$",       "AMDisbetter!" },
	{ "VENDORAMD$",        "AuthenticAMD" },
	{ "VENDORCENTAUR$",    "CentaurHauls" },
	{ "VENDORCYRIX$",      "CyrixInstead" },
	{ "VENDORINTEL$",      "GenuineIntel" },
	{ "VENDORTRANSMETA$",  "TransmetaCPU" },
	{ "VENDORTRANSMETAS$", "GenuineTMx86" },
	{ "VENDORNSC$",        "Geode by NSC" },
	{ "VENDORNEXGEN$",     "NexGenDriven" },
	{ "VENDORRISE$",       "RiseRiseRise" },
	{ "VENDORSIS$",        "SiS SiS SiS " },
	{ "VENDORUMC$",        "UMC UMC UMC " },
	{ "VENDORVIA$",        "VIA VIA VIA " },
	{ "VENDORVORTEX86$",   "Vortex86 SoC" },
	{ "VENDORZHAOXIN$",    "  Shanghai  " },
	{ "VENDORHYGON$",      "HygonGenuine" },
	{ "VENDORRDC$",        "Genuine  RDC" },
	{ "VENDORBHYVE$",      "bhyve bhyve " },
	{ "VENDORKVM$",        " KVMKVMKVM  " },
	{ "VENDORQEMU$",       "TCGTCGTCGTCG" },
	{ "VENDORHYPERV$",     "Microsoft Hv" },
	{ "VENDORXTA$",        "MicrosoftXTA" },
	{ "VENDORPARALLELS1$", " lrpepyh  vr" },
	{ "VENDORPARALLELS2$", "prl hyperv  " },
	{ "VENDORVMWARE$",     "VMwareVMware" },
	{ "VENDORXEN$",        "XenVMMXenVMM" },
	{ "VENDORACRN$",       "ACRNACRNACRN" },
	{ "VENDORQNX$",        " QNXQVMBSQG " },
	{ NULL,                NULL }
};

#define NEGATE_STATEMENT(s, len) { \
	char statement[len + 3]; \
	snprintf(statement, len + 3, "%s -", s); \
	if (strncmp(statement, p, len + 2) == 0) { \
		memcpy(d, p, len + 1); \
		d += len + 1; \
		p += len + 2; \
		memcpy(d, "(0-1)*", 6); \
		d += 6; \
	} \
}

char* clean_basic(const char* program, char* output_buffer)
{
	bool in_quotes = false;
	uint16_t bracket_depth = 0, count = 0;
	const char* p = program;
	char *d = output_buffer;
	while (*p) {
		NEGATE_STATEMENT("IF", 2);
		NEGATE_STATEMENT(" TO", 3);
		NEGATE_STATEMENT(" STEP", 5);
		NEGATE_STATEMENT("PRINT", 5);
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
				count++;
			} else {
				p++;
			}
		} else {
			*d++ = *p++;
			count++;
		}
		// Remove extra newlines
		if (*(p - 1) == '\n' || *(p - 1) == '\r') {
			while (*p == '\r' || *p == '\n') {
				p++;
			}
		}
		if (count > 2) {
			char* prev = d - 1;
			while (isspace(*prev) && prev > output_buffer) --prev;
			if (*(prev) == '-') {
				char* ptr = prev;
				--prev;
				while (isspace(*prev) && prev > output_buffer) --prev;
				const char op = *prev;
				/* Translate negative values into an expression represented entirely as positive numbers */
				if (op == '=' || op == '<' || op == '>' || op == '*' || op == '-' ||
					op == '/' || op == '+' || op == ',' || op == ';' || op == '(') {
					*ptr = '(';
					memcpy(d, "0-1)*", 5);
					d += 5;
					count += 5;
				}
			}
		}
	}
	// Terminate output
	*d = 0;
	//dprintf("clean => %s\n", output_buffer);
	return output_buffer;
}

void set_system_variables(struct basic_ctx* ctx, uint32_t pid)
{
	const struct g_cpuid_vendor* p = &cpuid_vendors[0];
	while (p->varname != NULL &&
		   p->vendor != NULL) {
		dprintf("'%s' -> '%s'\n", p->varname, p->vendor);
		basic_set_string_variable(p->varname, p->vendor, ctx, false, false);
		++p;
	}
	basic_set_int_variable("TRUE", 1, ctx, false, false);
	basic_set_int_variable("FALSE", 0, ctx, false, false);
	basic_set_int_variable("PID", pid, ctx, false, false);
	basic_set_double_variable("PI#", 3.141592653589793238, ctx, false, false);
	basic_set_double_variable("E#", 2.7182818284590451, ctx, false, false);
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

bool basic_hash_lines(struct basic_ctx* ctx, char** error)
{
	/* Build doubly linked list of line number references */
	uint64_t last_line = 0xFFFFFFFF;
	while (!tokenizer_finished(ctx)) {
		uint64_t line = tokenizer_num(ctx, NUMBER);
		if (last_line != 0xFFFFFFFF && (line <= last_line)) {
			*error = "Misordered lines in BASIC program";
			basic_destroy(ctx);
			return false;
		}
		last_line = line;
		if (hashmap_set(ctx->lines, &(ub_line_ref){ .line_number = line, .ptr = ctx->ptr })) {
			*error = "Line hashed twice in BASIC program (internal error)";
			basic_destroy(ctx);
			return false;
		}
		ctx->highest_line = line;
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
	return true;
}

struct basic_ctx* basic_init(const char *program, console* cons, uint32_t pid, const char* file, char** error)
{
	if (!isdigit(*program)) {
		/* Program is not line numbered! Auto-number it. */
		const char* numbered = auto_number(program, 10, 10);
		struct basic_ctx* c = basic_init(numbered, cons, pid, file, error);
		kfree(numbered);
		return c;
	}
	struct basic_ctx* ctx = kmalloc(sizeof(struct basic_ctx));
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
	basic_parse_fn(ctx);

	set_system_variables(ctx, pid);
	basic_set_string_variable("PROGRAM$", file, ctx, false, false);

	if (!basic_hash_lines(ctx, error)) {
		return false;
	}

	return ctx;
}

void yield_statement(struct basic_ctx* ctx)
{
	accept_or_return(YIELD, ctx);
	accept_or_return(NEWLINE, ctx);
	__asm__ volatile("hlt");
}

/**
 * @brief Loads a library by appending it to the end of a running program
 * This will cause many pointers in the context to become invalid, so the
 * library_statement() function will regenerate them. Do not rely on copies
 * of any ptrs inside ctx from before this call!
 * 
 * @param ctx BASIC context
 */
void library_statement(struct basic_ctx* ctx)
{
	accept_or_return(LIBRARY, ctx);
	const char* lib_file = str_expr(ctx);

	/* Validate the file exists and is not a directory */
	fs_directory_entry_t* file_info = fs_get_file_info(lib_file);
	if (!file_info || fs_is_directory(lib_file)) {
		tokenizer_error_print(ctx, "Not a library file");
	}
	accept_or_return(NEWLINE, ctx);
	/* Calculate the next line we will continue from after loading the library
	 * (we need to look ahead and take note of this because the entire program
	 * pointer structure will be rebuilt and any old ctx->ptr will be invalid!)
	 */
	uint64_t next_line = tokenizer_num(ctx, NUMBER);

	/* Load the library file from VFS */
	size_t library_len = file_info->size;
	char* temp_library = kmalloc(library_len);
	char* clean_library = kmalloc(library_len);
	if (!fs_read_file(file_info, 0, library_len, (uint8_t*)temp_library)) {
		tokenizer_error_print(ctx, "Error reading library file");
		kfree(temp_library);
		kfree(clean_library);
		return;
	}
	*(temp_library + library_len) = 0;

	/* Clean the BASIC code and check it is not numbered code */
	clean_library = clean_basic(temp_library, clean_library);
	kfree(temp_library);
	if (isdigit(*clean_library)) {
		tokenizer_error_print(ctx, "Library files cannot contain line numbers");
		kfree(clean_library);
		return;
	}

	/* Auto-number the library to be above the existing program statements */
	const char* numbered = auto_number(clean_library, ctx->highest_line + 10, 10);
	library_len = strlen(numbered);

	/* Append the renumbered library to the end of the program (this reallocates
	 * ctx->program_ptr invalidating ctx->ptr and ctx->next_ptr - the tokeinizer
	 * must be reinitailised and the line hash rebuilt)
	 */
	ctx->program_ptr = krealloc(ctx->program_ptr, strlen(ctx->program_ptr) + 5000 + library_len);
	strlcpy(ctx->program_ptr + strlen(ctx->program_ptr) - 1, numbered, strlen(ctx->program_ptr) + 5000 + library_len);
	kfree(clean_library);
	kfree(numbered);

	/* Reinitialise token parser and scan for new functions/procedures now the
	 * library is included in the program. Frees old list of DEFs.
	 */
	tokenizer_init(ctx->program_ptr, ctx);
	basic_free_defs(ctx);
	basic_parse_fn(ctx);

	/* Rebuild line number hash map (needs a complete rehash as all pointers are
	 * now invalidated)
	 */
	char error[MAX_STRINGLEN];
	hashmap_free(ctx->lines);
	ctx->lines = hashmap_new(sizeof(ub_line_ref), 0, 5923530135432, 458397058, line_hash, line_compare, NULL, NULL);
	if (!basic_hash_lines(ctx, (char**)&error)) {
		tokenizer_error_print(ctx, error);
		return;
	}

	/* Reset tokenizer again, and jump back to the line number after the LIBRARY
	 * statement that we recorded at the top of the function.
	 */
	tokenizer_init(ctx->program_ptr, ctx);
	jump_linenum(next_line, ctx);

}

struct basic_ctx* basic_clone(struct basic_ctx* old)
{
	struct basic_ctx* ctx = kmalloc(sizeof(struct basic_ctx));
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
	ctx->highest_line = old->highest_line;

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

void basic_parse_fn(struct basic_ctx* ctx)
{
	int currentline = 0;

	while (true) {
		currentline = tokenizer_num(ctx, NUMBER);
		char const* linestart = ctx->ptr;
		do {
			do {
				tokenizer_next(ctx);
			} while (tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT && *ctx->ptr);
			
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
		while (tokenizer_token(ctx) != NUMBER && tokenizer_token(ctx) != ENDOFINPUT && *ctx->ptr);

		if (tokenizer_token(ctx) == ENDOFINPUT || !*ctx->ptr) {
			break;
		}
	}

	tokenizer_init(ctx->program_ptr, ctx);

	ctx->ended = false;
	return;
}

struct ub_proc_fn_def* basic_find_fn(const char* name, struct basic_ctx* ctx)
{
	struct ub_proc_fn_def* cur = ctx->defs;
	for (; cur; cur = cur->next) {
		if (!strcmp(name, cur->name)) {
			return cur;
		}
	}
	return NULL;
}

void basic_free_defs(struct basic_ctx* ctx)
{
	for (; ctx->defs; ctx->defs = ctx->defs->next) {
		kfree(ctx->defs->name);
		for (; ctx->defs->params; ctx->defs->params = ctx->defs->params->next) {
			kfree(ctx->defs->params->name);
			kfree(ctx->defs->params);
		}
	}
	ctx->defs = NULL;
}

void basic_destroy(struct basic_ctx* ctx)
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
	basic_free_defs(ctx);
	kfree((char*)ctx->program_ptr);
	kfree(ctx);
}

bool accept(int token, struct basic_ctx* ctx)
{
	if (token != tokenizer_token(ctx)) {
		char err[MAX_STRINGLEN];
		GENERATE_ENUM_STRING_NAMES(TOKEN, token_names)
		sprintf(err, "Expected %s got %s", token_names[token], token_names[tokenizer_token(ctx)]);
		tokenizer_error_print(ctx, err);
		return false;
	}
	tokenizer_next(ctx);
	return true;
}

bool jump_linenum(int64_t linenum, struct basic_ctx* ctx)
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

void goto_statement(struct basic_ctx* ctx)
{
	accept_or_return(GOTO, ctx);
	jump_linenum(tokenizer_num(ctx, NUMBER), ctx);
}

void colour_statement(struct basic_ctx* ctx, int tok)
{
	accept_or_return(tok, ctx);
	setforeground((console*)ctx->cons, expr(ctx));
	accept_or_return(NEWLINE, ctx);
}

void background_statement(struct basic_ctx* ctx)
{
	accept_or_return(BACKGROUND, ctx);
	setbackground((console*)ctx->cons, expr(ctx));
	accept_or_return(NEWLINE, ctx);
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
void free_local_heap(struct basic_ctx* ctx)
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
void init_local_heap(struct basic_ctx* ctx)
{
	ctx->local_int_variables[ctx->call_stack_ptr] = NULL;
	ctx->local_string_variables[ctx->call_stack_ptr] = NULL;
	ctx->local_double_variables[ctx->call_stack_ptr] = NULL;
}


char* printable_syntax(struct basic_ctx* ctx)
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
		} else if (tokenizer_token(ctx) == OPENBRACKET || tokenizer_token(ctx) == VARIABLE || tokenizer_token(ctx) == NUMBER || tokenizer_token(ctx) == HEXNUMBER) {
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

void print_statement(struct basic_ctx* ctx)
{
	accept_or_return(PRINT, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		putstring((console*)ctx->cons, out);
	}
}

void sockwrite_statement(struct basic_ctx* ctx)
{
	int fd = -1;

	accept_or_return(SOCKWRITE, ctx);
	fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx), ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	const char* out = printable_syntax(ctx);
	if (out) {
		send(fd, out, strlen(out));
	}
}

void proc_statement(struct basic_ctx* ctx)
{
	char procname[MAX_STRINGLEN];
	char* p = procname;
	size_t procnamelen = 0;
	accept_or_return(PROC, ctx);
	while (*ctx->ptr != '\n' && *ctx->ptr != 0  && *ctx->ptr != '(' && procnamelen < MAX_STRINGLEN - 1) {
		if (*ctx->ptr != ' ') {
			*(p++) = *(ctx->ptr++);
		}
		procnamelen++;
	}
	*p++ = 0;
	struct ub_proc_fn_def* def = basic_find_fn(procname, ctx);
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
		accept_or_return(NEWLINE, ctx);

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

bool conditional(struct basic_ctx* ctx)
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

void else_statement(struct basic_ctx* ctx)
{
	/* If we get to an ELSE, this means that we executed a THEN part of a block IF,
	 * so we must skip it and any content up until the next ENDIF 
	 */
	accept_or_return(ELSE, ctx);
	accept_or_return(NEWLINE, ctx);
	while (tokenizer_token(ctx) != END && !tokenizer_finished(ctx)) {
		tokenizer_next(ctx);
		if (*ctx->ptr == 'I' && *(ctx->ptr+1) == 'F') {
			accept_or_return(IF, ctx);
			accept_or_return(NEWLINE, ctx);
			return;
		}
	}
	tokenizer_error_print(ctx, "Block IF/THEN/ELSE without ENDIF");
}

void if_statement(struct basic_ctx* ctx)
{
	accept_or_return(IF, ctx);
	bool r = conditional(ctx);
	accept_or_return(THEN, ctx);
	if (r) {
		if (tokenizer_token(ctx) == NEWLINE) {
			/* Multi-statement block IF */
			accept_or_return(NEWLINE, ctx);
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
					accept_or_return(NEWLINE, ctx);
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
					accept_or_return(IF, ctx);
					accept_or_return(NEWLINE, ctx);
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

void chain_statement(struct basic_ctx* ctx)
{
	accept_or_return(CHAIN, ctx);
	const char* pn = str_expr(ctx);
	process_t* p = proc_load(pn, ctx->cons, proc_cur()->pid, proc_cur()->csd);
	if (p == NULL) {
		accept_or_return(NEWLINE, ctx);
		return;
	}
	struct basic_ctx* new_proc = p->code;
	struct ub_var_int* cur_int = ctx->int_variables;
	struct ub_var_string* cur_str = ctx->str_variables;
	struct ub_var_double* cur_double = ctx->double_variables;

	/* Inherit global variables into new process */
	for (; cur_int; cur_int = cur_int->next) {
		if (cur_int->global) {
			basic_set_int_variable(cur_int->varname, cur_int->value, new_proc, false, true);
		}
	}
	for (; cur_str; cur_str = cur_str->next) {
		if (cur_str->global) {
			basic_set_string_variable(cur_str->varname, cur_str->value, new_proc, false, true);
		}
	}
	for (; cur_double; cur_double = cur_double->next) {
		if (cur_double->global) {
			basic_set_double_variable(cur_double->varname, cur_double->value, new_proc, false, true);
		}
	}

	proc_wait(proc_cur(), p->pid);
	accept_or_return(NEWLINE, ctx);
}

void eval_statement(struct basic_ctx* ctx)
{
	accept_or_return(EVAL, ctx);
	const char* v = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);

	if (ctx->current_linenum == EVAL_LINE) {
		ctx->eval_linenum = 0;
		tokenizer_error_print(ctx, "Recursive EVAL");
		return;
	}

	char clean_v[MAX_STRINGLEN];
	clean_basic(v, clean_v);

	if (ctx->oldlen == 0) {
		basic_set_string_variable("ERROR$", "", ctx, false, false);
		basic_set_int_variable("ERROR", 0, ctx, false, false);
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

void rem_statement(struct basic_ctx* ctx)
{
	accept_or_return(REM, ctx);
	while (*ctx->nextptr && *ctx->nextptr != '\n') ctx->nextptr++;
	tokenizer_next(ctx);
	accept_or_return(NEWLINE, ctx);
}

void def_statement(struct basic_ctx* ctx)
{
	// Because the function or procedure definition is pre-parsed by basic_init(),
	// we just skip the entire line moving to the next if we hit a DEF statement.
	// in the future we should check if the interpreter is actually calling a FN,
	// to check we dont fall through into a function.
	accept_or_return(DEF, ctx);
	while (*ctx->nextptr && *ctx->nextptr != '\n') ctx->nextptr++;
	tokenizer_next(ctx);
	accept_or_return(NEWLINE, ctx);
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
void input_statement(struct basic_ctx* ctx)
{
	accept_or_return(INPUT, ctx);
	const char* var = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);

	/* Clear buffer */
	if (kinput(10240, (console*)ctx->cons) != 0) {
		switch (var[strlen(var) - 1]) {
			case '$':
				basic_set_string_variable(var, kgetinput((console*)ctx->cons), ctx, false, false);
			break;
			case '#':
				double f = 0;
				atof(kgetinput((console*)ctx->cons), &f);
				basic_set_double_variable(var, f, ctx, false, false);
			break;
			default:
				basic_set_int_variable(var, atoll(kgetinput((console*)ctx->cons), 10), ctx, false, false);
			break;
		}
		kfreeinput((console*)ctx->cons);
		accept_or_return(NEWLINE, ctx);
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
void sockread_statement(struct basic_ctx* ctx)
{
	char input[MAX_STRINGLEN];
	const char* var = NULL;
	int fd = -1;

	accept_or_return(SOCKREAD, ctx);
	fd = basic_get_numeric_int_variable(tokenizer_variable_name(ctx), ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	var = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);

	int rv = recv(fd, input, MAX_STRINGLEN, false, 10);

	if (rv > 0) {
		*(input + rv) = 0;
		switch (var[strlen(var) - 1]) {
			case '$':
				basic_set_string_variable(var, input, ctx, false, false);
			break;
			case '#':
				double f = 0;
				atof(input, &f);
				basic_set_double_variable(var, f, ctx, false, false);
			break;
			default:
				basic_set_int_variable(var, atoll(input, 10), ctx, false, false);
			break;
		}

		accept_or_return(NEWLINE, ctx);
	} else if (rv < 0) {
		tokenizer_error_print(ctx, socket_error(rv));
	} else {
		jump_linenum(ctx->current_linenum, ctx);
	}
}

void connect_statement(struct basic_ctx* ctx)
{
	char input[MAX_STRINGLEN];
	const char* fd_var = NULL, *ip = NULL;
	int64_t port = 0;

	accept_or_return(CONNECT, ctx);
	fd_var = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	ip = str_expr(ctx);
	accept_or_return(COMMA, ctx);
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
				basic_set_int_variable(fd_var, rv, ctx, false, false);
			break;
		}

		accept_or_return(NEWLINE, ctx);
	} else {
		tokenizer_error_print(ctx, socket_error(rv));
	}
}

void sockclose_statement(struct basic_ctx* ctx)
{
	const char* fd_var = NULL;

	accept_or_return(SOCKCLOSE, ctx);
	fd_var = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);

	int rv = closesocket(basic_get_numeric_int_variable(fd_var, ctx));
	if (rv == 0) {
		// Clear variable to -1
		basic_set_int_variable(fd_var, -1, ctx, false, false);
		accept_or_return(NEWLINE, ctx);
	} else {
		tokenizer_error_print(ctx, socket_error(rv));
	}
}

void let_statement(struct basic_ctx* ctx, bool global, bool local)
{
	const char* var;
	const char* _expr;
	double f_expr = 0;

	var = tokenizer_variable_name(ctx);

	if (varname_is_int_array_access(ctx, var)) {
		int64_t index = arr_expr_set_index(ctx, var);
		int64_t value = expr(ctx);
		if (index == -1) {
			basic_set_int_array(var, value, ctx);
			accept_or_return(NEWLINE, ctx);
			return;
		}
		basic_set_int_array_variable(var, index, value, ctx);
		accept_or_return(NEWLINE, ctx);
		return;
	}
	if (varname_is_string_array_access(ctx, var)) {
		int64_t index = arr_expr_set_index(ctx, var);
		const char* value = str_expr(ctx);
		if (index == -1) {
			basic_set_string_array(var, value, ctx);
			accept_or_return(NEWLINE, ctx);
			return;
		}
		basic_set_string_array_variable(var, index, value, ctx);
		accept_or_return(NEWLINE, ctx);
		return;
	}
	if (varname_is_double_array_access(ctx, var)) {
		int64_t index = arr_expr_set_index(ctx, var);
		double value = 0;
		double_expr(ctx, &value);
		if (index == -1) {
			basic_set_double_array(var, value, ctx);
			accept_or_return(NEWLINE, ctx);
			return;
		}
		basic_set_double_array_variable(var, index, value, ctx);
		accept_or_return(NEWLINE, ctx);
		return;
	}

	accept_or_return(VARIABLE, ctx);
	accept_or_return(EQUALS, ctx);

	switch (var[strlen(var) - 1]) {
		case '$':
			_expr = str_expr(ctx);
			basic_set_string_variable(var, _expr, ctx, local, global);
		break;
		case '#':
			double_expr(ctx, &f_expr);
			basic_set_double_variable(var, f_expr, ctx, local, global);
		break;
		default:
			basic_set_int_variable(var, expr(ctx), ctx, local, global);
		break;
	}
	accept_or_return(NEWLINE, ctx);
}

void cls_statement(struct basic_ctx* ctx)
{
	accept_or_return(CLS, ctx);
	clearscreen(current_console);
	accept_or_return(NEWLINE, ctx);
}

void gcol_statement(struct basic_ctx* ctx)
{
	accept_or_return(GCOL, ctx);
	ctx->graphics_colour = expr(ctx);
	//dprintf("New graphics color: %08X\n", ctx->graphics_colour);
	accept_or_return(NEWLINE, ctx);
}

void gotoxy_statement(struct basic_ctx* ctx)
{
	accept_or_return(CURSOR, ctx);
	int64_t x = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y = expr(ctx);
	gotoxy(x, y);
	accept_or_return(NEWLINE, ctx);
}

void draw_line_statement(struct basic_ctx* ctx)
{
	accept_or_return(LINE, ctx);
	int64_t x1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	draw_line(x1, y1, x2, y2, ctx->graphics_colour);
}

void point_statement(struct basic_ctx* ctx)
{
	accept_or_return(POINT, ctx);
	int64_t x1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	putpixel(x1, y1, ctx->graphics_colour);
}

void triangle_statement(struct basic_ctx* ctx)
{
	accept_or_return(TRIANGLE, ctx);
	int64_t x1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x3 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y3 = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	draw_triangle(x1, y1, x2, y2, x3, y3, ctx->graphics_colour);
}

void rectangle_statement(struct basic_ctx* ctx)
{
	accept_or_return(RECTANGLE, ctx);
	int64_t x1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y1 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t x2 = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y2 = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	draw_horizontal_rectangle(x1, y1, x2, y2, ctx->graphics_colour);
}

void circle_statement(struct basic_ctx* ctx)
{
	accept_or_return(CIRCLE, ctx);
	int64_t x = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t y = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t radius = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t filled = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	draw_circle(x, y, radius, filled, ctx->graphics_colour);
}

void gosub_statement(struct basic_ctx* ctx)
{
	int linenum;

	accept_or_return(GOSUB, ctx);
	linenum = tokenizer_num(ctx, NUMBER);
	accept_or_return(NUMBER, ctx);
	accept_or_return(NEWLINE, ctx);

	if (ctx->call_stack_ptr < MAX_CALL_STACK_DEPTH) {
		ctx->call_stack[ctx->call_stack_ptr] = tokenizer_num(ctx, NUMBER);
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		jump_linenum(linenum, ctx);
	} else {
		tokenizer_error_print(ctx, "GOSUB: stack exhausted");
	}
}

void return_statement(struct basic_ctx* ctx)
{
	accept_or_return(RETURN, ctx);
	if (ctx->call_stack_ptr > 0) {
		free_local_heap(ctx);
		ctx->call_stack_ptr--;
		jump_linenum(ctx->call_stack[ctx->call_stack_ptr], ctx);
	} else {
		tokenizer_error_print(ctx, "RETURN without GOSUB");
	}
}

void next_statement(struct basic_ctx* ctx)
{
	accept_or_return(NEXT, ctx);
	if (ctx->for_stack_ptr > 0) {
		bool continue_loop = false;
		if (strchr(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, '#')) {
			double incr;
			basic_get_double_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ctx, &incr);
			incr += ctx->for_stack[ctx->for_stack_ptr - 1].step;
			basic_set_double_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, incr, ctx, false, false);
			continue_loop = ((ctx->for_stack[ctx->for_stack_ptr - 1].step > 0 && incr < ctx->for_stack[ctx->for_stack_ptr - 1].to) ||
			    (ctx->for_stack[ctx->for_stack_ptr - 1].step < 0 && incr > ctx->for_stack[ctx->for_stack_ptr - 1].to));
		} else {
			int64_t incr = basic_get_numeric_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, ctx);
			incr += ctx->for_stack[ctx->for_stack_ptr - 1].step;
			basic_set_int_variable(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable, incr, ctx, false, false);
			continue_loop = ((ctx->for_stack[ctx->for_stack_ptr - 1].step > 0 && incr < ctx->for_stack[ctx->for_stack_ptr - 1].to) ||
			    (ctx->for_stack[ctx->for_stack_ptr - 1].step < 0 && incr > ctx->for_stack[ctx->for_stack_ptr - 1].to));
		}
		if (continue_loop) {
			jump_linenum(ctx->for_stack[ctx->for_stack_ptr - 1].line_after_for, ctx);
		} else {
			kfree(ctx->for_stack[ctx->for_stack_ptr - 1].for_variable);
			ctx->for_stack_ptr--;
			accept_or_return(NEWLINE, ctx);
		}

	} else {
		tokenizer_error_print(ctx, "NEXT without FOR");
		accept_or_return(NEWLINE, ctx);
	}
}

void for_statement(struct basic_ctx* ctx)
{
	accept_or_return(FOR, ctx);
	const char* for_variable = strdup(tokenizer_variable_name(ctx));
	accept_or_return(VARIABLE, ctx);
	accept_or_return(EQUALS, ctx);
	if (strchr(for_variable, '#')) {
		double d;
		double_expr(ctx, &d);
		basic_set_double_variable(for_variable, d, ctx, false, false);
	} else {
		basic_set_int_variable(for_variable, expr(ctx), ctx, false, false);
	}
	accept_or_return(TO, ctx);
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
		accept_or_return(STEP, ctx);
		step = expr(ctx);
		accept_or_return(NEWLINE, ctx);
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

void repeat_statement(struct basic_ctx* ctx)
{
	accept_or_return(REPEAT, ctx);
	accept_or_return(NEWLINE, ctx);
	if (ctx->repeat_stack_ptr < MAX_LOOP_STACK_DEPTH) {
		ctx->repeat_stack[ctx->repeat_stack_ptr] = tokenizer_num(ctx, NUMBER);
		ctx->repeat_stack_ptr++;
	} else {
		tokenizer_error_print(ctx, "REPEAT stack exhausted");
	}

}


void until_statement(struct basic_ctx* ctx)
{
	accept_or_return(UNTIL, ctx);
	bool done = conditional(ctx);
	accept_or_return(NEWLINE, ctx);

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

void endif_statement(struct basic_ctx* ctx)
{
	if (ctx->if_nest_level == 0) {
		tokenizer_error_print(ctx, "ENDIF outside of block IF");
	}
	accept_or_return(IF, ctx);
	accept_or_return(NEWLINE, ctx);
	ctx->if_nest_level--;
}

void end_statement(struct basic_ctx* ctx)
{
	accept_or_return(END, ctx);
	if (tokenizer_token(ctx) == IF) {
		endif_statement(ctx);
		return;
	}
	ctx->ended = true;
}

void eq_statement(struct basic_ctx* ctx)
{
	accept_or_return(EQUALS, ctx);

	if (ctx->fn_type == RT_STRING) {
		const char* e = str_expr(ctx);
		ctx->fn_return = (void*)e;
	} else if (ctx->fn_type == RT_FLOAT) {
		double_expr(ctx, (void*)&ctx->fn_return);
	} else if (ctx->fn_type == RT_INT)  {
		ctx->fn_return = (void*)expr(ctx);
	} else if (ctx->fn_type == RT_NONE)  {
		tokenizer_error_print(ctx, "Can't return a value from a PROC");
		return;
	}

	accept_or_return(NEWLINE, ctx);

	ctx->ended = true;
}

void retproc_statement(struct basic_ctx* ctx)
{
	accept_or_return(RETPROC, ctx);
	accept_or_return(NEWLINE, ctx);
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

int64_t basic_asc(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("ASC", 0);
	return (unsigned char)*strval;
}

int64_t basic_val(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("VAL", 0);
	return atoll(strval, 10);
}

int64_t basic_hexval(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("HEXVAL", 0);
	return atoll(strval, 16);
}

int64_t basic_octval(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("OCTVAL", 0);
	return atoll(strval, 8);
}

void statement(struct basic_ctx* ctx)
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
		case CHDIR:
			return chdir_statement(ctx);
		case CIRCLE:
			return circle_statement(ctx);
		case LIBRARY:
			return library_statement(ctx);
		case YIELD:
			return yield_statement(ctx);
		case LET:
			accept_or_return(LET, ctx);
			/* Fall through. */
		case VARIABLE:
			return let_statement(ctx, false, false);
		case GLOBAL:
			accept_or_return(GLOBAL, ctx);
			return let_statement(ctx, true, false);
		case LOCAL:
			accept_or_return(LOCAL, ctx);
			return let_statement(ctx, false, true);
		case EQUALS:
			return eq_statement(ctx);
		default:
			return tokenizer_error_print(ctx, "Unknown keyword");
	}
}

void chdir_statement(struct basic_ctx* ctx)
{
	accept_or_return(CHDIR, ctx);
	const char* csd = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);
	const char* old = strdup(proc_cur()->csd);
	const char* new = proc_set_csd(proc_cur(), csd);
	if (new && fs_is_directory(new)) {
		kfree(old);
		return;
	}

	if (!new) {
		tokenizer_error_print(ctx, "Invalid directory");
	}
	if (!fs_is_directory(new)) {
		tokenizer_error_print(ctx, "Not a directory");
	}

	proc_set_csd(proc_cur(), old);
	kfree(old);
}

void line_statement(struct basic_ctx* ctx)
{
	ctx->current_linenum = tokenizer_num(ctx, NUMBER);
	accept_or_return(NUMBER, ctx);
	statement(ctx);
}

void basic_run(struct basic_ctx* ctx)
{
	if (basic_finished(ctx)) {
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

bool basic_finished(struct basic_ctx* ctx)
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
		if (!isalnum(*i)) {
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
	int ofs = 0;
	for (i = name; *i != '#'; i++) {
		if (*i == '#' && *(i + 1) != 0) {
		       return false;
		}
		if (ofs > 0 && isdigit(*i)) {
			continue;
		}
		if ((*i < 'A' || *i > 'Z') && (*i < 'a' || *i > 'z') && *i != '_') {
			return false;
		}
		if (ofs > 60) {
			return false;
		}
		ofs++;
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
	int ofs = 0;
	for (i = name; *i; i++) {
		if (ofs > 0 && isdigit(*i)) {
			continue;
		}
		if ((*i < 'A' || *i > 'Z') && (*i < 'a' || *i > 'z') && *i != '_') {
			return false;
		}
		if (ofs > 60) {
			return false;
		}
		ofs++;
	}
	return true;
}

void basic_set_string_variable(const char* var, const char* value, struct basic_ctx* ctx, bool local, bool global)
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

void basic_set_int_variable(const char* var, int64_t value, struct basic_ctx* ctx, bool local, bool global)
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
		struct ub_var_int* cur = local ? ctx->local_int_variables[ctx->call_stack_ptr] : ctx->int_variables;
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

void basic_set_double_variable(const char* var, double value, struct basic_ctx* ctx, bool local, bool global)
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

void begin_comma_list(struct ub_proc_fn_def* def, struct basic_ctx* ctx) {
	ctx->bracket_depth = 0;
	ctx->param = def->params;
	ctx->item_begin = (char*)ctx->ptr;
}


uint8_t extract_comma_list(struct ub_proc_fn_def* def, struct basic_ctx* ctx) {
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
				basic_set_string_variable(ctx->param->name, str_expr(ctx), ctx, true, false);
			} else if (ctx->param->name[len - 1] == '#') {
				double f = 0.0;
				double_expr(ctx, &f);
				//dprintf("double value\n");
				basic_set_double_variable(ctx->param->name, f, ctx, true, false);
			} else {
				//dprintf("int value\n");
				basic_set_int_variable(ctx->param->name, expr(ctx), ctx, true, false);
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

const char* basic_eval_str_fn(const char* fn_name, struct basic_ctx* ctx)
{
	struct ub_proc_fn_def* def = basic_find_fn(fn_name + 2, ctx);
	const char* rv = "";
	if (def) {
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct basic_ctx* atomic = basic_clone(ctx);
		atomic->fn_type = RT_STRING;
		jump_linenum(def->line, atomic);

		while (!basic_finished(atomic)) {
			line_statement(atomic);
			if (atomic->errored) {
				ctx->errored = true;
				break;
			}
		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
			rv = gc_strdup((const char*)atomic->fn_return);
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

int64_t basic_getproccount(struct basic_ctx* ctx)
{
	return proc_total();
}

int64_t basic_get_text_max_x(struct basic_ctx* ctx)
{
	return get_text_width();
}

int64_t basic_get_text_max_y(struct basic_ctx* ctx)
{
	return get_text_height();
}

int64_t basic_get_text_cur_x(struct basic_ctx* ctx)
{
	uint64_t x = 0, y = 0;
	get_text_position(&x, &y);
	return x;
}

int64_t basic_get_text_cur_y(struct basic_ctx* ctx)
{
	uint64_t x = 0, y = 0;
	get_text_position(&x, &y);
	return y;
}

int64_t basic_get_free_mem(struct basic_ctx* ctx)
{
	return get_free_memory();
}

int64_t basic_get_used_mem(struct basic_ctx* ctx)
{
	return get_used_memory();
}

int64_t basic_get_total_mem(struct basic_ctx* ctx)
{
	return get_total_memory();
}

int64_t basic_getprocid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCID", 0);
	return proc_id(intval);
}

int64_t basic_rgb(struct basic_ctx* ctx)
{
	int64_t r, g, b;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	r = intval;
	PARAMS_GET_ITEM(BIP_INT);
	g = intval;
	PARAMS_GET_ITEM(BIP_INT);
	b = intval;
	PARAMS_END("RGB", 0);
	return (uint32_t)(r << 16 | g << 8 | b);
}

char* basic_getprocname(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCNAME$","");
	process_t* process = proc_find(proc_id(intval));
	return process && process->name ? gc_strdup(process->name) : "";
}

int64_t basic_getprocparent(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCPARENT", 0);
	process_t* process = proc_find(proc_id(intval));
	return process ? process->ppid : 0;
}

int64_t basic_getproccpuid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCCPUID", 0);
	process_t* process = proc_find(proc_id(intval));
	return process ? process->cpu : 0;
}

char* basic_ramdisk_from_device(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("RAMDISK$","");
	const char* rd = init_ramdisk_from_storage(strval);
	if (!rd) {
		return "";
	}
	return gc_strdup(rd);
}

char* basic_ramdisk_from_size(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t blocks = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t block_size = intval;
	PARAMS_END("RAMDISK","");
	const char* rd = init_ramdisk(blocks, block_size);
	if (!rd) {
		return "";
	}
	return gc_strdup(rd);
}

char* basic_inkey(struct basic_ctx* ctx)
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

char* basic_insocket(struct basic_ctx* ctx)
{
	uint8_t input[2] = { 0, 0 };
	
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t fd = intval;
	PARAMS_END("INSOCKET$","");

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

int64_t basic_sockstatus(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t fd = intval;
	PARAMS_END("SOCKSTATUS", 0);

	if (fd < 0) {
		return 0;
	}

	return is_connected(fd);
}

int64_t basic_ctrlkey(struct basic_ctx* ctx)
{
	return ctrl_held();
}

int64_t basic_shiftkey(struct basic_ctx* ctx)
{
	return shift_held();
}

int64_t basic_altkey(struct basic_ctx* ctx)
{
	return alt_held();
}

int64_t basic_capslock(struct basic_ctx* ctx)
{
	return caps_lock_on();
}

int64_t basic_random(struct basic_ctx* ctx)
{
	int64_t low, high;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	low = intval;
	PARAMS_GET_ITEM(BIP_INT);
	high = intval;
	PARAMS_END("RND", 0);
	return (mt_rand() % (high - low + 1)) + low;
}


char* basic_chr(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("CHR$","");
	char res[2] = {(unsigned char)intval, 0};
	return gc_strdup(res);
}

char* basic_str(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("STR$","");
	char res[MAX_STRINGLEN];
	snprintf(res, MAX_STRINGLEN, "%lld", intval);
	return gc_strdup(res);
}

char* basic_bool(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("BOOL$","");
	char res[MAX_STRINGLEN];
	snprintf(res, MAX_STRINGLEN, "%s", intval ? "TRUE" : "FALSE");
	return gc_strdup(res);
}

int64_t basic_instr(struct basic_ctx* ctx)
{
	char* haystack;
	char* needle;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	haystack = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	needle = strval;
	PARAMS_END("INSTR", 0);
	size_t n_len = strlen(needle);
	for (size_t i = 0; i < strlen(haystack) - n_len + 1; ++i) {
		if (!strncmp(haystack + i, needle, n_len)) {
			return i + 1;
		}
	}
	return 0;
}

char* basic_netinfo(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("NETINFO$","");
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

char* basic_dns(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("DNS$","");
	char ip[16] = { 0 };
	uint32_t addr = dns_lookup_host(getdnsaddr(), strval, 2);
	get_ip_str(ip, (uint8_t*)&addr);
	return gc_strdup(ip);
}

char* basic_upper(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("UPPER$","");
	char* modified = gc_strdup(strval);
	for (char* m = modified; *m; ++m) {
		*m = toupper(*m);
	}
	return modified;
}

char* basic_lower(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("LOWER$","");
	char* modified = gc_strdup(strval);
	for (char* m = modified; *m; ++m) {
		*m = tolower(*m);
	}
	return modified;
}

char* basic_csd(struct basic_ctx* ctx)
{
	return gc_strdup(proc_cur()->csd);
}

char* basic_tokenize(struct basic_ctx* ctx)
{
	char* varname, *split;
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_VARIABLE);
	varname = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	split = strval;
	PARAMS_END("TOKENIZE$","");
	const char* current_value = basic_get_string_variable(varname, ctx);
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
			basic_set_string_variable(varname, new_value, ctx, false, false);
			return gc_strdup(return_value);
		}
		current_value++;
		ofs++;
	}
	char return_value[MAX_STRINGLEN];
	strlcpy(return_value, old_value, MAX_STRINGLEN);
	basic_set_string_variable(varname, "", ctx, false, false);
	return gc_strdup(return_value);
}


char* basic_left(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("LEFT$","");
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

char* basic_right(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("RIGHT$","");
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
	return gc_strdup(strval + len - intval);
}

char* basic_mid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_GET_ITEM(BIP_INT);
	int64_t start = intval;
	intval = 0;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t end = intval;
	PARAMS_END("MID$","");
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

int64_t basic_len(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("LEN",0);
	return strlen(strval);
}

int64_t basic_abs(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("ABS",0);
	return labs(intval);
}

void basic_sin(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("SIN");
	*res = sin(doubleval);
}

void basic_cos(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("COS");
	*res = cos(doubleval);
}

void basic_realval(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END_VOID("REALVAL");
	atof(strval, res);
	return;
}

void basic_tan(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("TAN");
	*res = tan(doubleval);
}

void basic_pow(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double base = doubleval;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	PARAMS_END_VOID("POW");
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
char basic_builtin_int_fn(const char* fn_name, struct basic_ctx* ctx, int64_t* res) {
	int i;
	for (i = 0; builtin_int[i].name; ++i) {
		if (!strcmp(fn_name, builtin_int[i].name)) {
			*res = builtin_int[i].handler(ctx);
			return 1;
		}
	}
	return 0;
}

char basic_builtin_double_fn(const char* fn_name, struct basic_ctx* ctx, double* res) {
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
char basic_builtin_str_fn(const char* fn_name, struct basic_ctx* ctx, char** res) {
	int i;
	for (i = 0; builtin_str[i].name; ++i) {
		if (!strcmp(fn_name, builtin_str[i].name)) {
			*res = builtin_str[i].handler(ctx);
			return 1;
		}
	}
	return 0;
}


int64_t basic_eval_int_fn(const char* fn_name, struct basic_ctx* ctx)
{
	struct ub_proc_fn_def* def = basic_find_fn(fn_name + 2, ctx);
	int64_t rv = 0;
	if (def) {
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct basic_ctx* atomic = basic_clone(ctx);
		atomic->fn_type = RT_INT;
		dprintf("Function eval, jump to line %d\n", def->line);
		jump_linenum(def->line, atomic);

		while (!basic_finished(atomic)) {
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

void basic_eval_double_fn(const char* fn_name, struct basic_ctx* ctx, double* res)
{
	struct ub_proc_fn_def* def = basic_find_fn(fn_name + 2, ctx);
	if (def) {
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct basic_ctx* atomic = basic_clone(ctx);
		atomic->fn_type = RT_FLOAT;
		jump_linenum(def->line, atomic);

		while (!basic_finished(atomic)) {
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

const char* basic_test_string_variable(const char* var, struct basic_ctx* ctx)
{
	struct ub_var_string* list[] = {
		ctx->local_string_variables[ctx->call_stack_ptr],
		ctx->str_variables
	};
	for (int j = 0; j < 2; j++) {
		struct ub_var_string* cur = list[j];
		for (; cur; cur = cur->next) {
			assert(cur->next != cur, "Variable list is linked to itself");
			assert(cur->varname, "NULL variable name");
			if (!strcmp(var, cur->varname))	{
				return cur->value;
			}
		}
	}
	return NULL;
}

const char* basic_get_string_variable(const char* var, struct basic_ctx* ctx)
{
	char* retv;
	int t = basic_builtin_str_fn(var, ctx, &retv);
	if (t)
		return retv;

	if (varname_is_string_function(var)) {
		const char* res = basic_eval_str_fn(var, ctx);
		return res;
	}

	if (varname_is_string_array_access(ctx, var)) {
		return basic_get_string_array_variable(var, arr_variable_index(ctx), ctx);
	}

	struct ub_var_string* list[ctx->call_stack_ptr + 1];
	int j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_string_variables[j];
	}
	list[0] = ctx->str_variables;
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_string* cur = list[j];
		for (; cur; cur = cur->next) {
			assert(cur->next != cur, "Variable list is linked to itself");
			assert(cur->varname, "NULL variable name");
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

bool basic_double_variable_exists(const char* var, struct basic_ctx* ctx)
{
	struct ub_var_double* list[ctx->call_stack_ptr + 1];
	int j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_double_variables[j];
	}
	list[0] = ctx->double_variables;
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_double* cur = list[j];
		for (; cur; cur = cur->next) {
			assert(cur->next != cur, "Variable list is linked to itself");
			assert(cur->varname, "NULL variable name");
			if (!strcmp(var, cur->varname))	{
				return true;
			}
		}
	}
	return false;
}

bool basic_string_variable_exists(const char* var, struct basic_ctx* ctx)
{
	struct ub_var_string* list[ctx->call_stack_ptr + 1];
	int j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_string_variables[j];
	}
	list[0] = ctx->str_variables;
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_string* cur = list[j];
		for (; cur; cur = cur->next) {
			assert(cur->next != cur, "Variable list is linked to itself");
			assert(cur->varname, "NULL variable name");
			if (!strcmp(var, cur->varname))	{
				return true;
			}
		}
	}
	return false;
}

bool basic_int_variable_exists(const char* var, struct basic_ctx* ctx)
{
	struct ub_var_int* list[ctx->call_stack_ptr + 1];
	int j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_int_variables[j];
	}
	list[0] = ctx->int_variables;
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_int* cur = list[j];
		for (; cur; cur = cur->next) {
			assert(cur->next != cur, "Variable list is linked to itself");
			assert(cur->varname, "NULL variable name");
			if (!strcmp(var, cur->varname))	{
				return true;
			}
		}
	}
	return false;
}

int64_t basic_get_int_variable(const char* var, struct basic_ctx* ctx)
{
	int64_t retv = 0;
	if (basic_builtin_int_fn(var, ctx, &retv)) {
		return retv;
	}
	
	if (varname_is_function(var)) {
		return basic_eval_int_fn(var, ctx);
	}

	if (varname_is_int_array_access(ctx, var)) {
		return basic_get_int_array_variable(var, arr_variable_index(ctx), ctx);
	}

	struct ub_var_int* list[ctx->call_stack_ptr + 1];
	int j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_int_variables[j];
	}
	list[0] = ctx->int_variables;
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_int* cur = list[j];
		for (; cur; cur = cur->next) {
			assert(cur->next != cur, "Variable list is linked to itself");
			assert(cur->varname, "NULL variable name");
			if (!strcmp(var, cur->varname))	{
				int64_t v = cur->value;
				/* If ERROR is read, it resets its value */
				if (!strcmp(var, "ERROR")) {
					basic_set_int_variable("ERROR", 0, ctx, false, false);
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

bool basic_get_double_variable(const char* var, struct basic_ctx* ctx, double* res)
{
	if (basic_builtin_double_fn(var, ctx, res)) {
		return true;
	}
		
	if (varname_is_double_function(var)) {
		basic_eval_double_fn(var, ctx, res);
		return true;
	}

	if (varname_is_double_array_access(ctx, var)) {
		return basic_get_double_array_variable(var, arr_variable_index(ctx), ctx, res);
	}


	struct ub_var_double* list[ctx->call_stack_ptr + 1];
	int j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_double_variables[j];
	}
	list[0] = ctx->double_variables;
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_double* cur = list[j];
		for (; cur; cur = cur->next) {
			assert(cur->next != cur, "Variable list is linked to itself");
			assert(cur->varname, "NULL variable name");
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

ub_return_type basic_get_numeric_variable(const char* var, struct basic_ctx* ctx, double* res)
{
	if (basic_get_double_variable(var, ctx, res)) {
		return RT_INT;
	}
	*res = (double)(basic_get_int_variable(var, ctx));
	return RT_FLOAT;
}

int64_t basic_get_numeric_int_variable(const char* var, struct basic_ctx* ctx)
{
	double res;
	if (basic_get_double_variable(var, ctx, &res)) {
		return (int64_t)res;
	}
	return basic_get_int_variable(var, ctx);
}

void write_cpuid(struct basic_ctx* ctx, int leaf)
{
	__cpuid(
		leaf,
		ctx->last_cpuid_result.eax,
		ctx->last_cpuid_result.ebx,
		ctx->last_cpuid_result.ecx,
		ctx->last_cpuid_result.edx);
}

void write_cpuidex(struct basic_ctx* ctx, int leaf, int subleaf)
{
	__cpuid_count(
		leaf,
		subleaf,
		ctx->last_cpuid_result.eax,
		ctx->last_cpuid_result.ebx,
		ctx->last_cpuid_result.ecx,
		ctx->last_cpuid_result.edx);
}

int64_t get_cpuid_reg(struct basic_ctx* ctx, int64_t reg)
{
	cpuid_result_t* res = &ctx->last_cpuid_result;
	switch (reg) {
	case 0:
		return res->eax;
	case 1:
		return res->ebx;
	case 2:
		return res->ecx;
	case 3:
		return res->edx;
	}
	tokenizer_error_print(ctx, "Invaild register");
	return 0;
}

int64_t basic_legacy_cpuid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t leaf = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t subleaf = intval;
	PARAMS_END("LEGACYCPUID", -1);
	if (subleaf != -1) {
		write_cpuidex(ctx, leaf, subleaf);
		return 1;
	}
	write_cpuid(ctx, leaf);
	return 0;
}

int64_t basic_legacy_getlastcpuid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("LEGACYGETLASTCPUID", -1);
	return get_cpuid_reg(ctx, intval);
}

char* basic_cpugetbrand(struct basic_ctx* ctx)
{

	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	bool trim = intval;
	PARAMS_END("CPUGETBRAND$", "");
	char buffer[50] = {0};
	unsigned int* tmp = (unsigned int*) buffer;
	__cpuid(
		0x80000002,
		tmp[0],
		tmp[1],
		tmp[2],
		tmp[3]
	);
	__cpuid(
		0x80000003,
		tmp[4],
		tmp[5],
		tmp[6],
		tmp[7]
	);
	__cpuid(
		0x80000004,
		tmp[8],
		tmp[9],
		tmp[10],
		tmp[11]
	);
	buffer[48] = 0;
	char* bufferp = (char*) buffer;
	if (trim) {
		while (*bufferp == ' ') {
			++bufferp;
		}
	}
	return gc_strdup(bufferp);
}

char* basic_cpugetvendor(struct basic_ctx* ctx)
{
	__cpuid(
		0,
		ctx->last_cpuid_result.eax,
		ctx->last_cpuid_result.ebx,
		ctx->last_cpuid_result.ecx,
		ctx->last_cpuid_result.edx);
	char buffer[13];
	((unsigned int*) buffer)[0] = ctx->last_cpuid_result.ebx;
	((unsigned int*) buffer)[1] = ctx->last_cpuid_result.edx;
	((unsigned int*) buffer)[2] = ctx->last_cpuid_result.ecx;
	buffer[12] = '\0';
	return gc_strdup(buffer);
}

char* basic_intoasc(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t target = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t length = intval;
	PARAMS_END("INTOASC$", "");
	if (length < 0 || length > 8) {
		tokenizer_error_print(ctx, "Invaild length");
		return gc_strdup("");
	}
	char result[16] = {0};
	(*(int64_t*) result) = target;
	result[length] = '\0';
	return gc_strdup(result);
}

int64_t basic_cpuid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t leaf = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t subleaf = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t reg = intval;
	PARAMS_END("CPUID", -1);
	if (subleaf != -1) {
		write_cpuidex(ctx, leaf, subleaf);
	} else {
		write_cpuid(ctx, leaf);
	}
	return get_cpuid_reg(ctx, reg);
}

void basic_sqrt(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_DOUBLE);
	double v = doubleval;
	PARAMS_END_VOID("SQRT");
	*res = sqrt(v);
}
