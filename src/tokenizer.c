/*
 * BBC BASIC interpreter,
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

#define DEBUG 0

#if DEBUG
#define DEBUG_PRINTF(...)	kprintf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#include <kernel.h>

#define MAX_NUMLEN 32

/**
 * @brief Contains a list of statement keywords.
 * 
 * @note It is important that these are alphabetically sorted.
 * It allows us to optimise the search.
 */
const int keywords[] = {
	CALL,
	CHAIN,
	CIRCLE,
	CLOSE,
	CLS,
	COLOR,
	COLOUR,
	CONNECT,
	CURSOR,
	DEF,
	DELETE,
	ELSE,
	END,
	EOF,
	EVAL,
	FOR,
	GCOL,
	GLOBAL,
	GOSUB,
	GOTO,
	IF,
	INPUT,
	LET,
	LINE,
	MKDIR,
	MOUNT,
	NEXT,
	OPENIN,
	OPENOUT,
	OPENUP,
	POINT,
	PRINT,
	READ,
	RECTANGLE,
	REM,
	RETURN,
	RMDIR,
	SOCKCLOSE,
	SOCKREAD,
	SOCKWRITE,
	STEP,
	THEN,
	TO,
	TRIANGLE,
	WRITE,
	-1,
};

/*---------------------------------------------------------------------------*/
static int singlechar(struct ubasic_ctx* ctx)
{
	switch (*ctx->ptr) {
		case '\n':
			return CR;
		case ',':
			return COMMA;
		case ';':
			return SEMICOLON;
		case '+':
			return PLUS;
		case '-':
			return MINUS;
		case '&':
			return AND;
		case '|':
			return OR;
		case '*':
			return ASTR;
		case '/':
			return SLASH;
		case '%':
			return MOD;
		case '(':
			return LEFTPAREN;
		case ')':
			return RIGHTPAREN;
		case '<':
			return LT;
		case '>':
			return GT;
		case '=':
			return EQ;
		case '~':
			return TILDE;
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
int get_next_token(struct ubasic_ctx* ctx)
{
	int i;
	char hex = 0;

	DEBUG_PRINTF("get_next_token(): '%s'\n", ctx->ptr);

	if(*ctx->ptr == 0) {
		return ENDOFINPUT;
	}
	
	if (isdigit(*ctx->ptr) || *ctx->ptr == '&'|| *ctx->ptr == '.') {
		hex = (*ctx->ptr == '&') ? 1 : 0;
		if (hex) {
			ctx->ptr++;
			for (i = 0; i < MAX_NUMLEN; ++i) {
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
			for (i = 0; i < MAX_NUMLEN; ++i) {
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
	} else if(singlechar(ctx)) {
		ctx->nextptr = ctx->ptr + 1;
		return singlechar(ctx);
	} else if(*ctx->ptr == '"') {
		ctx->nextptr = ctx->ptr;
		int strl = 0;
		do {
			++ctx->nextptr;
			if (++strl > 10240)
			{
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

	if ((*ctx->ptr >= 'a' && *ctx->ptr <= 'z') || (*ctx->ptr >= 'A' && *ctx->ptr <= 'Z') || (*ctx->ptr == '$') || (*ctx->ptr == '#')) {
		ctx->nextptr = ctx->ptr;
		int varl = 0;
		while ((*ctx->nextptr >= 'a' && *ctx->nextptr <= 'z') || (*ctx->nextptr >= 'A' && *ctx->nextptr <= 'Z') || (*ctx->nextptr == '$') || (*ctx->nextptr == '#') || (*ctx->nextptr == '(')
				|| (varl > 0 && isdigit(*ctx->nextptr)))
		{
			//kprintf("%c", *ctx->nextptr);
			ctx->nextptr++;
			if (*ctx->nextptr == '(')
			{
			//kprintf("Nextptr found open bracket\n");
			int bracketdepth = 1;
			do
			{
				ctx->nextptr++;
				if (*ctx->nextptr == '(')
					bracketdepth++;
				else if (*ctx->nextptr == ')')
					bracketdepth--;
			}
			while (bracketdepth > 1 && *ctx->nextptr != 0);
			}
			else
			if (++varl > 60)
			{
				tokenizer_error_print(ctx, "Variable name too long");
				break;
			}
		}
		if (*ctx->nextptr == '$' || *ctx->nextptr == '#')
			ctx->nextptr++;
		//kprintf("Variable. nextptr = %08x ptr = %08x\n", ctx->nextptr, ctx->ptr);
		return VARIABLE;
	}

	
	return ERROR;
}
/*---------------------------------------------------------------------------*/
void
tokenizer_init(const char *program, struct ubasic_ctx* ctx)
{
	ctx->ptr = program;
	ctx->current_token = get_next_token(ctx);
}
/*---------------------------------------------------------------------------*/
int tokenizer_token(struct ubasic_ctx* ctx)
{
	return ctx->current_token;
}
/*---------------------------------------------------------------------------*/
void tokenizer_next(struct ubasic_ctx* ctx)
{

	if(tokenizer_finished(ctx)) {
		return;
	}

	DEBUG_PRINTF("tokenizer_next: %p\n", ctx->nextptr);
	ctx->ptr = ctx->nextptr;
	while(*ctx->ptr == ' ') {
		++ctx->ptr;
	}
	ctx->current_token = get_next_token(ctx);
	DEBUG_PRINTF("tokenizer_next: '%s' %d\n", ctx->ptr, ctx->current_token);
	return;
}
/*---------------------------------------------------------------------------*/
int64_t tokenizer_num(struct ubasic_ctx* ctx, int token)
{
	return token == NUMBER ? atoll(ctx->ptr, 10) : atoll(ctx->ptr, 16);
}

void tokenizer_fnum(struct ubasic_ctx* ctx, int token, double* f)
{
	atof(ctx->ptr, f);
	return;
}
/*---------------------------------------------------------------------------*/
bool tokenizer_string(char *dest, int len, struct ubasic_ctx* ctx)
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
/*---------------------------------------------------------------------------*/
void tokenizer_error_print(struct ubasic_ctx* ctx, const char* error)
{
	ubasic_set_string_variable("ERROR$", error, ctx, false, false);
	ubasic_set_int_variable("ERROR", 1, ctx, false, false);
	ubasic_set_int_variable("ERRORLINE", ctx->current_linenum, ctx, false, false);
	if (ctx->eval_linenum == 0) {
		if (ctx->ended == 0) {
			setforeground(current_console, COLOUR_LIGHTRED);
			kprintf("Error on line %d: %s\n", ctx->current_linenum, error);
			setforeground(current_console, COLOUR_WHITE);
			ctx->ended = 1;
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
/*---------------------------------------------------------------------------*/
int tokenizer_finished(struct ubasic_ctx* ctx)
{
	//kprintf("tokenizer_finished? %d %08x\n", *ctx->ptr, ctx->ptr);
	return *ctx->ptr == 0 || ctx->current_token == ENDOFINPUT;
}
/*---------------------------------------------------------------------------*/
const char* tokenizer_variable_name(struct ubasic_ctx* ctx)
{
	char varname[MAX_VARNAME];
	int count = 0;
	while (((*ctx->ptr >= 'a' && *ctx->ptr <= 'z') || (*ctx->ptr >= 'A' && *ctx->ptr <= 'Z') || (*ctx->ptr == '$') || (*ctx->ptr == '#')) && count < MAX_VARNAME)
	{
		varname[count++] = *(ctx->ptr++);
	}
	varname[count] = 0;
	/* TODO: Validate variable name, weed out e.g. TEST$$ or $TEST$ which are not valid. */
	return gc_strdup(varname);
}
/*---------------------------------------------------------------------------*/

bool tokenizer_decimal_number(struct ubasic_ctx* ctx)
{
	const char* ptr = ctx->ptr;
	int whole_part_count = 0, decimal_part_count = 0;
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