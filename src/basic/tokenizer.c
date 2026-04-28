/**
 * @file basic/tokenizer.c
 * @brief BASIC tokenizer functions
 * 
 * Retro Rocket OS Project (C) Craig Edwards 2012.
 * loosely based on uBASIC (Copyright (c) 2006, Adam Dunkels, All rights reserved).
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
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

extern bool debug;

#define MAX_NUMLEN 32

/**
 * @brief Contains a list of statement keywords.
 * 
 * @note It is important that these are alphabetically sorted.
 * It allows us to optimise the search.
 *
 * This is sorted at link-time by the linker script into an
 * .rodata section called .kw
 */
TOKEN(EMIT_FROM_FLAG)
static const int kw_sentinel __attribute__((section(".kw.~zzzzzz"), used)) = -1;
extern const int __start_kw_array[];
extern const int __stop_kw_array[];
static const int *keywords = __start_kw_array;
static bool keyword_prefix_ready = false;

#define PREFIX_BUCKETS 65536

/**
 * @brief Two-character prefix index into the sorted keyword table.
 *
 * Maps 16-bit prefixes (derived from the first two characters of each keyword)
 * to offsets in `keywords[]`, allowing lookup to scan only the relevant subset
 * for a given prefix. Each entry defines the start of a bucket; the next entry
 * defines its end (half-open range).
 *
 * The prefix encoding preserves lexicographic ordering, so the linker-sorted
 * keyword table forms contiguous ranges per prefix.
 *
 * The final entry is a sentinel containing the total keyword count.
 */
static int keyword_prefix_offsets[PREFIX_BUCKETS + 1];

struct interned_name {
	const char *name;
	size_t name_length;
};

static struct hashmap *interned_variable_names = NULL;
static buddy_allocator_t interned_variable_name_allocator;
static bool interned_variable_name_allocator_ready = false;

static void *interned_name_map_malloc(size_t size, void *udata)
{
	[[maybe_unused]] void *unused = udata;
	return buddy_malloc(&interned_variable_name_allocator, size);
}

static void *interned_name_map_realloc(void *ptr, size_t size, void *udata)
{
	return buddy_realloc(&interned_variable_name_allocator, ptr, size);
}

static void interned_name_map_free(const void *ptr, void *udata)
{
	buddy_free(&interned_variable_name_allocator, ptr);
}

static uint64_t interned_name_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const struct interned_name *entry = item;
	size_t len = entry->name_length ? entry->name_length : strlen(entry->name);

	return hashmap_sip(entry->name, len, seed0, seed1);
}

static int interned_name_compare(const void *a, const void *b, void *udata)
{
	const struct interned_name *ea = a;
	const struct interned_name *eb = b;
	size_t la = ea->name_length ? ea->name_length : strlen(ea->name);
	size_t lb = eb->name_length ? eb->name_length : strlen(eb->name);

	if (la < lb) {
		return -1;
	}
	if (la > lb) {
		return 1;
	}
	return strcmp(ea->name, eb->name);
}

static bool ensure_interned_variable_names(void)
{
	if (!interned_variable_name_allocator_ready) {
		buddy_init(&interned_variable_name_allocator, 6, 22, 22);
		interned_variable_name_allocator_ready = true;
	}

	if (interned_variable_names) {
		return true;
	}

	interned_variable_names = hashmap_new_with_allocator(
		interned_name_map_malloc,
		interned_name_map_realloc,
		interned_name_map_free,
		sizeof(struct interned_name),
		1024,
		0x9e3779b97f4a7c15ULL,
		0xc2b2ae3d27d4eb4fULL,
		interned_name_hash,
		interned_name_compare,
		NULL,
		NULL
	);

	return interned_variable_names != NULL;
}

static const char *intern_variable_name(const char *name, size_t len)
{
	struct interned_name *existing;
	struct interned_name entry;
	char *copy;

	if (!ensure_interned_variable_names()) {
		return NULL;
	}

	existing = hashmap_get(
		interned_variable_names,
		&(struct interned_name) { .name = name, .name_length = len }
	);

	if (existing) {
		return existing->name;
	}

	copy = buddy_strdup(&interned_variable_name_allocator, name);
	if (!copy) {
		return NULL;
	}

	entry.name = copy;
	entry.name_length = len;

	hashmap_set(interned_variable_names, &entry);
	if (hashmap_oom(interned_variable_names)) {
		buddy_free(&interned_variable_name_allocator, copy);
		return NULL;
	}

	existing = hashmap_get(
		interned_variable_names,
		&(struct interned_name) { .name = name, .name_length = len }
	);

	return existing ? existing->name : copy;
}

/**
 * @brief Return a 16-bit lexicographic prefix for a string.
 *
 * Encodes the first two characters as (c0 << 8) | c1 so that the numeric
 * ordering of the key matches the alphabetical ordering of the keyword table.
 *
 * Assumptions:
 * - Strings are at least one character long (second byte may be '\0')
 * - Direct byte access is safe (x86-64, no alignment restrictions of concern)
 * - Endianness is known and controlled (explicit shift/or avoids host order)
 *
 * Portability is not a goal; this is tuned specifically for x86-64.
 */
static inline __attribute__((always_inline)) uint16_t prefix16(const char *p)
{
	return ((uint16_t)(unsigned char)p[0] << 8) | (uint16_t)(unsigned char)p[1];
}

void build_keyword_prefix_offsets(void)
{
	GENERATE_ENUM_STRING_NAMES(TOKEN, token_names)
	int keyword_count = 0;

	if (keyword_prefix_ready) {
		return;
	}

	dprintf("Building keyword buckets...\n");

	while (keywords[keyword_count] != -1) {
		keyword_count++;
	}

	/* Initialise all to sentinel */
	for (int i = 0; i <= PREFIX_BUCKETS; i++) {
		keyword_prefix_offsets[i] = keyword_count;
	}

	/* Record first occurrence */
	for (int i = 0; i < keyword_count; i++) {
		int tok = keywords[i];
		const char *name = token_names[tok];

		uint16_t key = prefix16(name);

		if (keyword_prefix_offsets[key] == keyword_count) {
			keyword_prefix_offsets[key] = i;
		}
	}

	/* Fill gaps */
	for (int i = PREFIX_BUCKETS - 1; i >= 0; i--) {
		if (keyword_prefix_offsets[i] == keyword_count) {
			keyword_prefix_offsets[i] = keyword_prefix_offsets[i + 1];
		}
	}

	int populated = 0;
	int max_bucket = 0;
	int max_key = 0;
	int total_entries = 0;

	for (int k = 0; k < PREFIX_BUCKETS; k++) {
	    int start = keyword_prefix_offsets[k];
	    int end   = keyword_prefix_offsets[k + 1];
	    int size  = end - start;

	    if (size > 0) {
		populated++;
		total_entries += size;

		if (size > max_bucket) {
		    max_bucket = size;
		    max_key = k;
		}
	    }
	}

	double mean = populated ? (double)total_entries / (double)populated : 0.0;
	char mean_s[128];
	double_to_string(mean, mean_s, sizeof(mean_s), 3);
	dprintf("Keyword bucket summary:\n");
	dprintf("  keywords        : %d\n", keyword_count);
	dprintf("  buckets used    : %d / %d\n", populated, PREFIX_BUCKETS);
	dprintf("  empty buckets   : %d\n", PREFIX_BUCKETS - populated);
	dprintf("  avg bucket size : %s\n", mean_s);
	dprintf("  max bucket size : %d (key=%04x '%c%c')\n", max_bucket, max_key, (char)(max_key >> 8), (char)(max_key & 0xff));

	keyword_prefix_ready = true;
}

static const enum token_t singlechar_tokens[256] = {
	['\n'] = NEWLINE,
	[','] = COMMA,
	[';'] = SEMICOLON,
	['+'] = PLUS,
	['-'] = MINUS,
	['*'] = ASTERISK,
	['/'] = SLASH,
	['%'] = MOD,
	['('] = OPENBRACKET,
	[')'] = CLOSEBRACKET,
	['<'] = LESSTHAN,
	['>'] = GREATERTHAN,
	['='] = EQUALS,
	['~'] = TILDE,
	[' '] = SPACE,
	['\t'] = SPACE,
};

static inline __attribute__((always_inline)) enum token_t singlechar(const char *p) {
	return singlechar_tokens[(unsigned char)*p];
}

int get_next_token(struct basic_ctx* ctx)
{
	enum token_t tok;

	if (*ctx->ptr == 0) {
		return ENDOFINPUT;
	}
	
	if (isdigit(*ctx->ptr) || *ctx->ptr == '&' || *ctx->ptr == '.') {
		if (*ctx->ptr == '&') {
			ctx->ptr++;
			for (int i = 0; i < MAX_NUMLEN; ++i) {
				if (!isxdigit(ctx->ptr[i])) {
					if (i > 1) {
						ctx->nextptr = ctx->ptr + i;
						return HEXNUMBER;
					} else {
						tokenizer_error_print(ctx, "Hexadecimal number too short");
						return NO_TOKEN;
					}
				}
			}
		} else {
			/* Scan forwards up to MAX_NUMLEN characters */
			for (int i = 0; i < MAX_NUMLEN; ++i) {
				/* Until we find a character that isn't part of a number */
				if (!isdigit(ctx->ptr[i]) && ctx->ptr[i] != '.') {
					if (i > 0) {
						ctx->nextptr = ctx->ptr + i;
						return NUMBER;
					} else {
						tokenizer_error_print(ctx, "Number too short");
						return NO_TOKEN;
					}
				}
			}
		}
		tokenizer_error_print(ctx, "Number too long");
		return NO_TOKEN;
	} else if ((tok = singlechar(ctx->ptr)) != NO_TOKEN) {
		ctx->nextptr = ctx->ptr + 1;
		return tok;
	} else if (*ctx->ptr == '"') {
		ctx->nextptr = ctx->ptr;
		do {
			++ctx->nextptr;
			if (*ctx->nextptr == 0 || *ctx->nextptr == 13) {
				tokenizer_error_printf(ctx, "Unterminated string constant");
				break;
			}
		} while (*ctx->nextptr != '"');
		++ctx->nextptr;
		return STRING;
	} else {
		GENERATE_ENUM_STRING_NAMES(TOKEN, token_names)
		GENERATE_ENUM_STRING_LENGTHS(TOKEN, token_name_lengths)
		uint16_t key = prefix16(ctx->ptr);
		int start = keyword_prefix_offsets[key];
		int end = keyword_prefix_offsets[key + 1];
		for (int kt = start; kt < end; ++kt) {
			tok = keywords[kt];
			size_t len = token_name_lengths[tok];
			/* First two characters already matched by prefix16 bucket.
			 * Compare from byte 2 onwards; <=2 length is already a full match.
			 * Ordering is preserved within the bucket, so early-exit still works.
			 */
			int comparison = len > 2 ? strncmp(ctx->ptr + 2, token_names[tok] + 2, len - 2) : 0;
			if (comparison == 0) {
				const char *backup = ctx->nextptr;
				ctx->nextptr = ctx->ptr + len;
				bool next_is_varlike = (*ctx->nextptr == '_' || isalnum(*ctx->nextptr));
				if (!next_is_varlike || tok == PROC || tok == FN || tok == EQUALS) {
					/* Only return the token if what follows the token is not continuation of a variable-name or keyword-name like sequence, e.g. "END -> ENDING"
					 * Special case for PROC, FN, =, as PROC and FN can be immediately followed by the name of their subroutine. e.g. PROCfoo
					 */
					return tok;
				} else {
					ctx->nextptr = backup;
				}
			} else if (comparison < 0) {
				/* We depend upon keyword_tokens being alphabetically sorted,
				 * so that we can bail early if we go too far down the list
				 * and still haven't found the keyword.
				 */
				break;
			}
		}
	}

	if ((*ctx->ptr >= 'a' && *ctx->ptr <= 'z') || (*ctx->ptr >= 'A' && *ctx->ptr <= 'Z') || *ctx->ptr == '_') {
		ctx->nextptr = ctx->ptr;
		int varl = 0;
		while (
			(*ctx->nextptr >= 'a' && *ctx->nextptr <= 'z') ||
			(*ctx->nextptr >= 'A' && *ctx->nextptr <= 'Z') ||
			(*ctx->nextptr == '_') ||
			(varl > 0 && *ctx->nextptr == '$') ||
			(varl > 0 && *ctx->nextptr == '#') ||
			(varl > 0 && *ctx->nextptr == '(') ||
			(varl > 0 && isdigit(*ctx->nextptr))
		) {
			ctx->nextptr++;
			if (*ctx->nextptr == '(') {
				int bracketdepth = 1;
				do {
					ctx->nextptr++;
					if (*ctx->nextptr == '(') {
						bracketdepth++;
					}
					else if (*ctx->nextptr == ')') {
						bracketdepth--;
					}
				}
				while (bracketdepth > 1 && *ctx->nextptr != 0);
			} else if (++varl > 60) {
				tokenizer_error_print(ctx, "Variable name too long");
				break;
			}
		}
		if (*ctx->nextptr == '$' || *ctx->nextptr == '#') {
			ctx->nextptr++;
		}
		return VARIABLE;
	}

	ctx->nextptr++;
	return NO_TOKEN;
}

void tokenizer_init(const char *program, struct basic_ctx* ctx)
{
	ctx->ptr = program;
	ctx->current_token = get_next_token(ctx);
}

enum token_t tokenizer_token(struct basic_ctx* ctx)
{
	return ctx->current_token;
}

void tokenizer_next(struct basic_ctx* ctx)
{
	if (tokenizer_finished(ctx)) {
		return;
	}

	ctx->ptr = ctx->nextptr;
	while (*ctx->ptr == ' ' || *ctx->ptr == '\t') {
		++ctx->ptr;
	}
	ctx->current_token = get_next_token(ctx);
}

int64_t tokenizer_num(struct basic_ctx* ctx, enum token_t token)
{
	return token == NUMBER ? atoll(ctx->ptr, 10) : atoll(ctx->ptr, 16);
}

void tokenizer_fnum(struct basic_ctx* ctx, enum token_t token, double* f)
{
	atof(ctx->ptr, f);
}

void tokenizer_error_printf(struct basic_ctx* ctx, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	__builtin_va_copy(args_copy, args);
	int needed = vsnprintf(NULL, 0, fmt, args_copy);
	va_end(args_copy);
	if (needed < 0) {
		va_end(args);
		tokenizer_error_print(ctx, "Error formatting error message");
		return;
	}
	char error[needed + 1];
	vsnprintf(error, needed + 1, fmt, args);
	va_end(args);
	tokenizer_error_print(ctx, error);
}

void tokenizer_error_print(struct basic_ctx* ctx, const char* error)
{
	dprintf("tokenizer_error_print: %s\n", error);
	basic_set_string_variable("ERR$", error, ctx, false, false);
	basic_set_int_variable("ERR", 1, ctx, false, false);
	basic_set_int_variable("ERRLINE", ctx->current_linenum, ctx, false, false);
	if (ctx->eval_linenum == 0) {
		if (ctx->ended == 0) {
			debug = false;
			if (ctx->error_handler) {
				dprintf("Handled error\n");
				struct ub_proc_fn_def* def = basic_find_fn(ctx->error_handler, ctx);
				if (def && ctx->call_stack_ptr < MAX_CALL_STACK_DEPTH) {
					buddy_free(ctx->allocator, ctx->error_handler);
					ctx->error_handler = NULL;
					if (!new_stack_frame(ctx)) {
						return;
					}
					init_local_heap(ctx);
					pop_stack_frame(ctx);
					ctx->fn_type_stack[ctx->call_stack_ptr] = ctx->fn_type; // save caller’s type
					ctx->fn_type = RT_NONE;
					/*Move to next line */
					while (tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT) {
						tokenizer_next(ctx);
					}
					accept_or_return(NEWLINE, ctx);
					/* Return point is the line after the error */
					ctx->call_stack[ctx->call_stack_ptr] = tokenizer_num(ctx, NUMBER);
					if (!new_stack_frame(ctx)) {
						return;
					}
					if (!jump_linenum(def->line, ctx)) {
						setforeground(COLOUR_LIGHTRED);
						kprintf("Error on line %ld: Unable to call error handler 'PROC%s', missing line %lu\n", ctx->current_linenum, ctx->error_handler, def->line);
						setforeground(COLOUR_WHITE);
						ctx->ended = true;
						return;
					}
					return;
				} else {
					setforeground(COLOUR_LIGHTRED);
					kprintf("Error on line %ld: Unable to call error handler 'PROC%s'\n", ctx->current_linenum, ctx->error_handler);
					setforeground(COLOUR_WHITE);
					ctx->ended = true;
				}
			}
			dprintf("Unhandled error\n");
			if (ctx->claimed_flip) {
				set_video_auto_flip(true);
			}
			ctx->ended = true;
			ctx->errored = true;
			setforeground(COLOUR_LIGHTRED);
			kprintf("Error on line %ld: %s\n", ctx->current_linenum, error);
			setforeground(COLOUR_DARKRED);
			ub_line_ref* line = hashmap_get(ctx->lines, &(ub_line_ref){ .line_number = ctx->current_linenum });
			if (line) {
				char l[MAX_STRINGLEN];
				char* p = strchr(line->ptr, '\n');
				strlcpy(l, line->ptr, p ? p - line->ptr + 1 : strlen(line->ptr));
				size_t offset = ctx->ptr - line->ptr;
				if (offset > strlen(l)) {
					offset = 1;
				}
				kprintf("%s\n", l);
				for (size_t x = 0; x < offset ? offset - 1 : 0; ++x) {
					put(' ');
				}
				kprintf("^\n");
			}
			setforeground(COLOUR_WHITE);
		}
	} else {
		dprintf("Error in eval\n");
		if (!ctx->errored) {
			ctx->errored = true;
			setforeground(COLOUR_LIGHTRED);
			kprintf("%s\n", error);
			setforeground(COLOUR_WHITE);
			jump_linenum(ctx->eval_linenum, ctx);
		}
	}
}

bool tokenizer_finished(struct basic_ctx* ctx)
{
	return *ctx->ptr == 0 || ctx->current_token == ENDOFINPUT;
}

const char* tokenizer_variable_name(struct basic_ctx* ctx, size_t* count)
{
	char varname[MAX_VARNAME];
	*count = 0;

	while (*count < MAX_VARNAME && *ctx->ptr != 0) {
		char c = *ctx->ptr;

		if (*count == 0) {
			if (!(isalpha(c) || c == '_')) {
				break;
			}
		} else {
			if (c == '$' || c == '#') {
				varname[(*count)++] = c;
				ctx->ptr++;

				if (*ctx->ptr != 0 && (isalnum(*ctx->ptr) || *ctx->ptr == '_' || *ctx->ptr == '$' || *ctx->ptr == '#')) {
					varname[*count] = 0;
					tokenizer_error_printf(ctx, "Invalid variable name '%s'", varname);
					*count = 0;
					return "";
				}
				break;
			}

			if (!(isalnum(c) || c == '_')) {
				break;
			}
		}

		varname[(*count)++] = c;
		ctx->ptr++;
	}

	varname[*count] = 0;

	const char* interned = intern_variable_name(varname, *count);
	if (!interned) {
		tokenizer_error_printf(ctx, "Out of memory interning variable name '%s'", varname);
		*count = 0;
		return "";
	}

	return interned;
}

bool tokenizer_decimal_number(struct basic_ctx* ctx)
{
	const char* ptr = ctx->ptr;
	int whole_part_count = 0, decimal_part_count = 0;
	if (*ptr == '+' || *ptr == '-') {
		ptr++;
	}
	while (isdigit(*ptr)) {
		whole_part_count++;
		ptr++;
	}
	if (whole_part_count && *ptr == '.') {
		ptr++;
		while (isdigit(*ptr)) {
			ptr++;
			decimal_part_count++;
		}
		if (decimal_part_count) {
			return true;
		}
	}
	return false;
}
