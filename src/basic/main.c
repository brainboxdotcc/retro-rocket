/**
 * @file basic/main.c
 * @brief Main functions for BASIC interpreter, context creation, destruction and cloning

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
#include <debugger.h>

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
	return output_buffer;
}

const char* auto_number(const char* program, uint64_t line, uint64_t increment)
{
	size_t new_size_max = strlen(program) * 5;
	char* newprog = kmalloc(new_size_max);
	char line_buffer[MAX_STRINGLEN];
	char* line_ptr = line_buffer;
	bool insert_line = true, ended = false;
	if (!newprog) {
		return NULL;
	}
	*newprog = 0;
	while (true) {
		if (insert_line) {
			while (*program == '\n') {
				*line_ptr++ = '\n';
				program++;
			}
			snprintf(line_buffer, MAX_STRINGLEN, "%lu ", line);
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
	kfree_null(&newprog);
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
	char* program = ctx->ptr;
	while (*program) {
		uint64_t line = atoi(program);
		if (last_line != 0xFFFFFFFF && (line <= last_line)) {
			*error = "Misordered lines in BASIC program";
			basic_destroy(ctx);
			return false;
		}
		last_line = line;
		if (hashmap_set(ctx->lines, &(ub_line_ref){ .line_number = line, .ptr = program })) {
			*error = "Line hashed twice in BASIC program (internal error)";
			basic_destroy(ctx);
			return false;
		}
		ctx->highest_line = line;
		while (*program && *program != '\n') {
			program++;
		}
		while (*program == '\n') {
			program++; // skip newline to start of next one
		}
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
		kfree_null(&numbered);
		return c;
	}
	struct basic_ctx* ctx = kmalloc(sizeof(struct basic_ctx));
	if (ctx == NULL) {
		*error = "Out of memory";
		return NULL;
	}
	int i;

	ctx->debug_status = 0;
	ctx->debug_breakpoints = NULL;
	ctx->debug_breakpoint_count = 0;
	ctx->if_nest_level = 0;
	ctx->errored = false;
	ctx->claimed_flip = false;
	ctx->current_token = ERROR;
	ctx->int_variables = NULL;
	memset(ctx->sprites, 0, sizeof(ctx->sprites));
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
		kfree_null(&ctx);
		*error = "Out of memory";
		return NULL;
	}
	ctx->string_gc_storage = kmalloc(STRING_GC_AREA_SIZE);
	if (!ctx->string_gc_storage) {
		kfree_null(&ctx);
		kfree_null(&ctx->program_ptr);
		*error = "Out of memory";
		return NULL;
	}
	/* Special for empty string storage */
	*ctx->string_gc_storage = 0;
	ctx->string_gc_storage_next = ctx->string_gc_storage + 1;
	buddy_init(&ctx->allocator, 6, 20, 20);
	ctx->lines = hashmap_new(sizeof(ub_line_ref), 0, 5923530135432, 458397058, line_hash, line_compare, NULL, NULL);

	// Clean extra whitespace from the program
	ctx->program_ptr = clean_basic(program, ctx->program_ptr);

	ctx->for_stack_ptr = ctx->call_stack_ptr = ctx->repeat_stack_ptr = 0;
	ctx->defs = NULL;

	ctx->graphics_colour = 0xFFFFFF;

	// Scan the program for functions and procedures

	tokenizer_init(ctx->program_ptr, ctx);
	if (!basic_parse_fn(ctx)) {
		*error = "Duplicate function name";
		return NULL;
	}

	tokenizer_init(ctx->program_ptr, ctx);
	set_system_variables(ctx, pid);
	basic_set_string_variable("PROGRAM$", file, ctx, false, false);

	if (!basic_hash_lines(ctx, error)) {
		dprintf("Failed to hash lines\n");
		return NULL;
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
	accept_or_return(NEWLINE, ctx);
	if (!file_info || fs_is_directory(lib_file)) {
		tokenizer_error_printf(ctx, "Not a library file: '%s'", lib_file);
		return;
	}
	/* Calculate the next line we will continue from after loading the library
	 * (we need to look ahead and take note of this because the entire program
	 * pointer structure will be rebuilt and any old ctx->ptr will be invalid!)
	 */
	uint64_t next_line = tokenizer_num(ctx, NUMBER);

	/* Load the library file from VFS */
	size_t library_len = file_info->size;
	char* temp_library = kmalloc(library_len + 1);
	if (!temp_library) {
		tokenizer_error_printf(ctx, "Not enough memory to load library file '%s'", lib_file);
		return;
	}
	char* clean_library = kmalloc(library_len + 1);
	if (!clean_library) {
		kfree_null(&temp_library);
		tokenizer_error_printf(ctx, "Not enough memory to load library file '%s'", lib_file);
		return;
	}
	if (!fs_read_file(file_info, 0, library_len, (uint8_t*)temp_library)) {
		tokenizer_error_printf(ctx, "Error reading library file '%s'", lib_file);
		kfree_null(&temp_library);
		kfree_null(&clean_library);
		return;
	}
	*(temp_library + library_len) = 0;

	/* Clean the BASIC code and check it is not numbered code */
	clean_library = clean_basic(temp_library, clean_library);
	kfree_null(&temp_library);
	if (isdigit(*clean_library)) {
		tokenizer_error_printf(ctx, "Library '%s': Library files cannot contain line numbers", lib_file);
		kfree_null(&clean_library);
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
	if (!ctx->program_ptr) {
		tokenizer_error_printf(ctx, "Not enough memory to load library file '%s'", lib_file);
		kfree_null(&numbered);
		return;
	}
	char last = ctx->program_ptr[strlen(ctx->program_ptr) - 2];
	if (last > 13) {
		strlcat(ctx->program_ptr, "\n", strlen(ctx->program_ptr) + 5000 + library_len);
	}
	strlcpy(ctx->program_ptr + strlen(ctx->program_ptr) - 1, numbered, strlen(ctx->program_ptr) + 5000 + library_len);
	kfree_null(&clean_library);
	kfree_null(&numbered);

	/* Reinitialise token parser and scan for new functions/procedures now the
	 * library is included in the program. Frees old list of DEFs.
	 */
	tokenizer_init(ctx->program_ptr, ctx);
	basic_free_defs(ctx);
	if (!basic_parse_fn(ctx)) {
		return;
	}

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

	/* Look for constructor PROC with same name as library */
	struct ub_proc_fn_def* def = basic_find_fn(file_info->filename, ctx);
	if (def) {
		dprintf("Calling initialisation constructor '%s' on line %ld with return to line %ld\n", file_info->filename, def->line, next_line);
		if (ctx->call_stack_ptr < MAX_CALL_STACK_DEPTH) {
			ctx->call_stack[ctx->call_stack_ptr] = next_line;
			init_local_heap(ctx);
			ctx->call_stack_ptr++;
			ctx->fn_type = RT_NONE;
			jump_linenum(def->line, ctx);
		} else {
			tokenizer_error_printf(ctx, "Call stack exhausted when calling constructor PROC for library '%s'", lib_file);
		}
		return;
	} else {
		dprintf("Library '%s' has no initialisation constructor, continue at line %ld\n", file_info->filename, next_line);
		jump_linenum(next_line, ctx);
	}
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
	ctx->string_gc_storage = old->string_gc_storage;
	ctx->string_gc_storage_next = old->string_gc_storage_next;
	ctx->lines = old->lines;
	ctx->highest_line = old->highest_line;
	ctx->debug_status = old->debug_status;
	ctx->debug_breakpoints = old->debug_breakpoints;
	ctx->debug_breakpoint_count = old->debug_breakpoint_count;
	memcpy(ctx->sprites, old->sprites, sizeof(ctx->sprites));

	for (i = 0; i < MAX_CALL_STACK_DEPTH; i++) {
		ctx->local_int_variables[i] = old->local_int_variables[i];
	}
	for (i = 0; i < MAX_CALL_STACK_DEPTH; i++) {
		ctx->local_string_variables[i] = old->local_string_variables[i];
	}
	for (i = 0; i < MAX_CALL_STACK_DEPTH; i++) {
		ctx->local_double_variables[i] = old->local_double_variables[i];
	}
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
	ctx->claimed_flip = old->claimed_flip;
	ctx->ended = false;
	ctx->errored = false;

	tokenizer_init(ctx->program_ptr, ctx);

	return ctx;
}


void basic_destroy(struct basic_ctx* ctx)
{
	for (; ctx->int_variables; ) {
		void* next = ctx->int_variables->next;
		kfree_null(&ctx->int_variables->varname);
		kfree_null(&ctx->int_variables);
		ctx->int_variables = next;
	}
	for (; ctx->double_variables; ) {
		void* next = ctx->double_variables->next;
		kfree_null(&ctx->double_variables->varname);
		kfree_null(&ctx->double_variables);
		ctx->double_variables = next;
	}
	for (; ctx->str_variables; ) {
		void* next = ctx->str_variables->next;
		kfree_null(&ctx->str_variables->varname);
		kfree_null(&ctx->str_variables->value);
		kfree_null(&ctx->str_variables);
		ctx->str_variables = next;
	}
	for (; ctx->int_array_variables; ) {
		void* next = ctx->int_array_variables->next;
		kfree_null(&ctx->int_array_variables->varname);
		kfree_null(&ctx->int_array_variables->values);
		kfree_null(&ctx->int_array_variables);
		ctx->int_array_variables = next;
	}
	for (; ctx->double_array_variables; ) {
		void* next = ctx->double_array_variables->next;
		kfree_null(&ctx->double_array_variables->varname);
		kfree_null(&ctx->double_array_variables->values);
		kfree_null(&ctx->double_array_variables);
		ctx->double_array_variables = next;
	}
	for (; ctx->string_array_variables; ) {
		void* next = ctx->string_array_variables->next;
		kfree_null(&ctx->string_array_variables->varname);
		for (size_t f = 0; f < ctx->string_array_variables->itemcount; ++f) {
			kfree_null(&ctx->string_array_variables->values[f]);
		}
		kfree_null(&ctx->string_array_variables);
		ctx->string_array_variables = next;
	}
	for (uint32_t sprite_handle = 0; sprite_handle < MAX_SPRITES; ++sprite_handle) {
		if (ctx->sprites[sprite_handle]) {
			free_sprite(ctx, sprite_handle);
		}
	}
	for (size_t x = 0; x < ctx->call_stack_ptr; x++) {
		for (; ctx->local_int_variables[x]; ) {
			void* next = ctx->local_int_variables[x]->next;
			kfree_null(&ctx->local_int_variables[x]->varname);
			kfree_null(&ctx->local_int_variables[x]);
			ctx->local_int_variables[x] = next;
		}
		for (; ctx->local_double_variables[x]; ) {
			void* next = ctx->local_double_variables[x]->next;
			kfree_null(&ctx->local_double_variables[x]->varname);
			kfree_null(&ctx->local_double_variables[x]);
			ctx->local_double_variables[x] = next;
		}
		for (; ctx->local_string_variables[x]; ) {
			void* next = ctx->local_string_variables[x]->next;
			kfree_null(&ctx->local_string_variables[x]->varname);
			kfree_null(&ctx->local_string_variables[x]->value);
			kfree_null(&ctx->local_string_variables[x]);
			ctx->local_string_variables[x] = next;
		}
	}
	kfree_null(&ctx->string_gc_storage);
	ctx->string_gc_storage_next = NULL;
	hashmap_free(ctx->lines);
	basic_free_defs(ctx);
	buddy_destroy(&ctx->allocator);
	kfree_null(&ctx->program_ptr);
	kfree_null(&ctx);
}

bool accept(int token, struct basic_ctx* ctx)
{
	if (token != tokenizer_token(ctx)) {
		GENERATE_ENUM_STRING_NAMES(TOKEN, token_names)
		tokenizer_error_printf(ctx, "Expected %s got %s", token_names[token], token_names[tokenizer_token(ctx)]);
		return false;
	}
	tokenizer_next(ctx);
	return true;
}

bool jump_linenum(int64_t linenum, struct basic_ctx* ctx)
{
	ub_line_ref* line = hashmap_get(ctx->lines, &(ub_line_ref){ .line_number = linenum });
	if (!line) {
		tokenizer_error_printf(ctx, "No such line %ld", linenum);
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

/**
 * @brief Free variables held on the local call stack
 * 
 * @param ctx BASIC context
 */
void free_local_heap(struct basic_ctx* ctx)
{
	while (ctx->local_string_variables[ctx->call_stack_ptr]) {
		struct ub_var_string* next = ctx->local_string_variables[ctx->call_stack_ptr]->next;
		kfree_null(&ctx->local_string_variables[ctx->call_stack_ptr]->value);
		kfree_null(&ctx->local_string_variables[ctx->call_stack_ptr]->varname);
		kfree_null(&ctx->local_string_variables[ctx->call_stack_ptr]);
		ctx->local_string_variables[ctx->call_stack_ptr] = next;
	}
	while (ctx->local_int_variables[ctx->call_stack_ptr]) {
		struct ub_var_int* next = ctx->local_int_variables[ctx->call_stack_ptr]->next;
		kfree_null(&ctx->local_int_variables[ctx->call_stack_ptr]->varname);
		kfree_null(&ctx->local_int_variables[ctx->call_stack_ptr]);
		ctx->local_int_variables[ctx->call_stack_ptr] = next;
	}
	while (ctx->local_double_variables[ctx->call_stack_ptr]) {
		struct ub_var_double* next = ctx->local_double_variables[ctx->call_stack_ptr]->next;
		kfree_null(&ctx->local_double_variables[ctx->call_stack_ptr]->varname);
		kfree_null(&ctx->local_double_variables[ctx->call_stack_ptr]);
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

void chain_statement(struct basic_ctx* ctx)
{
	uint32_t cpu = logical_cpu_id();
	process_t* proc = proc_cur(cpu);
	accept_or_return(CHAIN, ctx);
	const char* pn = str_expr(ctx);
	process_t* p = proc_load(pn, ctx->cons, proc->pid, proc->csd);
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

	proc_wait(proc, p->pid);
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

	if (strlen(clean_v) == 23 && gdb_trace(clean_v) == GDB_TRIGGER) {
		gdb_emit();
		return;
	}

	if (ctx->oldlen == 0) {
		basic_set_string_variable("ERROR$", "", ctx, false, false);
		basic_set_int_variable("ERROR", 0, ctx, false, false);
		ctx->oldlen = strlen(ctx->program_ptr);
		/* If program doesn't end in newline, add one */
		char last = ctx->program_ptr[strlen(ctx->program_ptr) - 2];
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
		case DATA:
			return data_statement(ctx);
		case RESTORE:
			return restore_statement(ctx);
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
		case SETVARI:
			return setvari_statement(ctx);
		case SETVARR:
			return setvarr_statement(ctx);
		case SETVARS:
			return setvars_statement(ctx);
		case PLOT:
			return plot_statement(ctx);
		case AUTOFLIP:
			return autoflip_statement(ctx);
		case FLIP:
			return flip_statement(ctx);
		case KEYMAP:
			return keymap_statement(ctx);
		case SPRITELOAD:
			return loadsprite_statement(ctx);
		case SPRITEFREE:
			return freesprite_statement(ctx);
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

void line_statement(struct basic_ctx* ctx)
{
	ctx->current_linenum = tokenizer_num(ctx, NUMBER);
	accept_or_return(NUMBER, ctx);
	if (tokenizer_token(ctx) == NEWLINE) {
		/* Empty line! */
		return;
	}
	statement(ctx);
}

void basic_run(struct basic_ctx* ctx)
{
	if (basic_finished(ctx)) {
		return;
	}
	line_statement(ctx);
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
	gc(ctx);
}

bool basic_finished(struct basic_ctx* ctx)
{
	return ctx->ended || tokenizer_finished(ctx);
}
