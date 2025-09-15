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

static int singlechar(struct basic_ctx* ctx)
{
	switch (*ctx->ptr) {
		case '\n':
			return NEWLINE;
		case ',':
			return COMMA;
		case ';':
			return SEMICOLON;
		case '+':
			return PLUS;
		case '-':
			return MINUS;
		case '*':
			return ASTERISK;
		case '/':
			return SLASH;
		case '%':
			return MOD;
		case '(':
			return OPENBRACKET;
		case ')':
			return CLOSEBRACKET;
		case '<':
			return LESSTHAN;
		case '>':
			return GREATERTHAN;
		case '=':
			return EQUALS;
		case '~':
			return TILDE;
	}
	return 0;
}

int get_next_token(struct basic_ctx* ctx)
{
	if(*ctx->ptr == 0) {
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
				if (!isxdigit(ctx->ptr[i])) {
					tokenizer_error_print(ctx, "Malformed hexadecimal number");
					return NO_TOKEN;
				}
			}
		} else {
			/* Scan forwards up to MAX_NUMLEN characters */
			for (int i = 0; i < MAX_NUMLEN; ++i) {
				/* Until we find a character that isnt part of a number */
				if (!isdigit(ctx->ptr[i]) && ctx->ptr[i] != '.') {
					if (i > 0) {
						ctx->nextptr = ctx->ptr + i;
						return NUMBER;
					} else {
						tokenizer_error_print(ctx, "Number too short");
						return NO_TOKEN;
					}
				}
				if (!isdigit(ctx->ptr[i]) && ctx->ptr[i] != '.') {
					tokenizer_error_print(ctx, "Malformed number");
					return NO_TOKEN;
				}
			}
		}
		tokenizer_error_print(ctx, "Number too long");
		return NO_TOKEN;
	} else if (singlechar(ctx)) {
		ctx->nextptr = ctx->ptr + 1;
		return singlechar(ctx);
	} else if (*ctx->ptr == '"') {
		ctx->nextptr = ctx->ptr;
		int strl = 0;
		do {
			++ctx->nextptr;
			if (++strl > MAX_STRINGLEN) {
				tokenizer_error_print(ctx, "String constant too long");
				break;
			}
		} while(*ctx->nextptr != '"');
		++ctx->nextptr;
		return STRING;
	} else {
		GENERATE_ENUM_STRING_NAMES(TOKEN, token_names)
		GENERATE_ENUM_STRING_LENGTHS(TOKEN, token_name_lengths)
		for(int kt = 0; keywords[kt] != -1; ++kt) {
			size_t len = token_name_lengths[keywords[kt]];
			int comparison = strncmp(ctx->ptr, token_names[keywords[kt]], len);
			if (comparison == 0) {
				const char* backup = ctx->nextptr;
				ctx->nextptr = ctx->ptr + len;
				bool next_is_varlike = ((*ctx->nextptr >= '0' && *ctx->nextptr <= '9') || (toupper(*ctx->nextptr) >= 'A' && toupper(*ctx->nextptr) <= 'Z') || *ctx->nextptr == '_');
				if (!next_is_varlike || keywords[kt] == PROC || keywords[kt] == FN || keywords[kt] == EQUALS) {
					/* Only return the token if what follows the token is not continuation of a variable-name or keyword-name like sequence, e.g. "END -> ENDING"
					 * Special case for PROC, FN, =, as PROC and FN can be immediately followed by the name of their subroutine. e.g. PROCfoo
					 */
					return keywords[kt];
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

	if(tokenizer_finished(ctx)) {
		return;
	}

	ctx->ptr = ctx->nextptr;
	while(*ctx->ptr == ' ' || *ctx->ptr == '\t') {
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

bool tokenizer_string(char *dest, int len, struct basic_ctx* ctx)
{
	char *string_end;
	int string_len;
	
	if(tokenizer_token(ctx) != STRING) {
		return true;
	}
	string_end = strchr(ctx->ptr + 1, '"');
	if(string_end == NULL) {
		tokenizer_error_print(ctx, "Unterminated \"");
		*dest = 0;
		return false;
	}
	string_len = string_end - ctx->ptr - 1;
	if(len < string_len) {
		string_len = len;
	}
	if (ctx->ptr == ctx->program_ptr) {
		tokenizer_error_print(ctx, "Unterminated \"");
		*dest = 0;
		return false;
	}
	memcpy(dest, ctx->ptr + 1, string_len);
	dest[string_len] = 0;
	return true;
}


void tokenizer_error_printf(struct basic_ctx* ctx, const char* fmt, ...)
{
	char error[MAX_STRINGLEN];
	va_list args;
	va_start(args, fmt);
	vsnprintf(error, MAX_STRINGLEN - 1, fmt, args);
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
					ctx->call_stack_ptr++;
					init_local_heap(ctx);
					ctx->call_stack_ptr--;
					ctx->fn_type_stack[ctx->call_stack_ptr] = ctx->fn_type; // save caller’s type
					ctx->fn_type = RT_NONE;
					/*Move to next line */
					while (tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT) {
						tokenizer_next(ctx);
					}
					accept_or_return(NEWLINE, ctx);
					/* Return point is the line after the error */
					ctx->call_stack[ctx->call_stack_ptr] = tokenizer_num(ctx, NUMBER);
					ctx->call_stack_ptr++;
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
	while (
		(
			(*ctx->ptr >= 'a' && *ctx->ptr <= 'z') ||
			(*ctx->ptr >= 'A' && *ctx->ptr <= 'Z') ||
			(*count > 0 && *ctx->ptr == '$') ||
			(*count > 0 && *ctx->ptr == '#') ||
			(*ctx->ptr == '_') ||
			(*count > 0 && isdigit(*ctx->ptr))
		) && *count < MAX_VARNAME
	) {
		varname[(*count)++] = *(ctx->ptr++);
	}
	varname[*count] = 0;
	for (size_t n = 0; n < *count - 1; ++n) {
		if (varname[n] == '$' || varname[n] == '#') {
			tokenizer_error_printf(ctx, "Invalid variable name '%s'", varname);
			*count = 0;
			return "";
		}
	}
	/* TODO: Validate variable name, weed out e.g. TEST$$ or $TEST$ which are not valid. */
	return gc_strdup(ctx, varname);
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
