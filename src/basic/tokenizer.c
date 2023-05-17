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

#define MAX_NUMLEN 32

/**
 * @brief Contains a list of statement keywords.
 * 
 * @note It is important that these are alphabetically sorted.
 * It allows us to optimise the search.
 */
const int keywords[] = {
	AND,
	BACKGROUND,
	CALL,
	CHAIN,
	CHDIR,
	CIRCLE,
	CLOSE,
	CLS,
	COLOR,
	COLOUR,
	CONNECT,
	CURSOR,
	DEF,
	DELETE,
	DIM,
	ELSE,
	END,
	EOF,
	EOR,
	EVAL,
	FOR,
	GCOL,
	GLOBAL,
	GOSUB,
	GOTO,
	IF,
	INPUT,
	LET,
	LIBRARY,
	LINE,
	LOCAL,
	MKDIR,
	MOUNT,
	NEXT,
	NOT,
	OPENIN,
	OPENOUT,
	OPENUP,
	OR,
	POINT,
	POP,
	PRINT,
	PROC,
	PUSH,
	READ,
	RECTANGLE,
	REDIM,
	REM,
	REPEAT,
	RETPROC,
	RETURN,
	RMDIR,
	SETVARI,
	SETVARR,
	SETVARS,
	SOCKCLOSE,
	SOCKREAD,
	SOCKWRITE,
	STEP,
	THEN,
	TO,
	TRIANGLE,
	UNTIL,
	WRITE,
	YIELD,
	-1,
};

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
						return ERROR;
					}
				}
				if (!isxdigit(ctx->ptr[i])) {
					tokenizer_error_print(ctx, "Malformed hexadecimal number");
					return ERROR;
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
						return ERROR;
					}
				}
				if (!isdigit(ctx->ptr[i]) && ctx->ptr[i] != '.') {
					tokenizer_error_print(ctx, "Malformed number");
					return ERROR;
				}
			}
		}
		tokenizer_error_print(ctx, "Number too long");
		return ERROR;
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
		for(int kt = 0; keywords[kt] != -1; ++kt) {
			size_t len = strlen(token_names[keywords[kt]]);
			int comparison = strncmp(ctx->ptr, token_names[keywords[kt]], len);
			if (comparison == 0) {
				ctx->nextptr = ctx->ptr + len;
				return keywords[kt];
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

	
	return ERROR;
}

void tokenizer_init(const char *program, struct basic_ctx* ctx)
{
	ctx->ptr = program;
	ctx->current_token = get_next_token(ctx);
}

int tokenizer_token(struct basic_ctx* ctx)
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

int64_t tokenizer_num(struct basic_ctx* ctx, int token)
{
	return token == NUMBER ? atoll(ctx->ptr, 10) : atoll(ctx->ptr, 16);
}

void tokenizer_fnum(struct basic_ctx* ctx, int token, double* f)
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

void tokenizer_error_print(struct basic_ctx* ctx, const char* error)
{
	basic_set_string_variable("ERROR$", error, ctx, false, false);
	basic_set_int_variable("ERROR", 1, ctx, false, false);
	basic_set_int_variable("ERRORLINE", ctx->current_linenum, ctx, false, false);
	if (ctx->eval_linenum == 0) {
		if (ctx->ended == 0) {
			setforeground(current_console, COLOUR_LIGHTRED);
			kprintf("Error on line %d: %s\n", ctx->current_linenum, error);
			setforeground(current_console, COLOUR_WHITE);
			ctx->ended = true;
		}
	} else {
		if (!ctx->errored) {
			ctx->errored = true;
			setforeground(current_console, COLOUR_LIGHTRED);
			kprintf("%s\n", error);
			setforeground(current_console, COLOUR_WHITE);
			jump_linenum(ctx->eval_linenum, ctx);
		}
	}
}

int tokenizer_finished(struct basic_ctx* ctx)
{
	return *ctx->ptr == 0 || ctx->current_token == ENDOFINPUT;
}

const char* tokenizer_variable_name(struct basic_ctx* ctx)
{
	char varname[MAX_VARNAME];
	int count = 0;
	while (
		(
			(*ctx->ptr >= 'a' && *ctx->ptr <= 'z') ||
			(*ctx->ptr >= 'A' && *ctx->ptr <= 'Z') ||
			(count > 0 && *ctx->ptr == '$') ||
			(count > 0 && *ctx->ptr == '#') ||
			(*ctx->ptr == '_') ||
			(count > 0 && isdigit(*ctx->ptr))
		) && count < MAX_VARNAME
	) {
		varname[count++] = *(ctx->ptr++);
	}
	varname[count] = 0;
	for (int n = 0; n < count - 1; ++n) {
		if (varname[n] == '$' || varname[n] == '#') {
			char error[MAX_STRINGLEN];
			snprintf(error, MAX_STRINGLEN, "Invalid variable name '%s'", varname);
			tokenizer_error_print(ctx, error);
			return "";
		}
	}
	/* TODO: Validate variable name, weed out e.g. TEST$$ or $TEST$ which are not valid. */
	return gc_strdup(varname);
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
