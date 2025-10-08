/**
 * @file basic/string.c
 * @brief BASIC string manipulation functions
 */
#include <kernel.h>
#include "regex.h"

struct match_state {
	struct regex_prog *prog;
	const uint8_t *hay;
	size_t hay_len;
	size_t off;
	struct regex_match m;
	int done;          /* 0 running, 1 finished */
	int matched;       /* 0/1 */
	size_t next_off;
	size_t fuel_per_tick;
};

extern struct basic_int_fn builtin_int[];
extern struct basic_str_fn builtin_str[];
extern struct basic_double_fn builtin_double[];

bool match_idle_step(process_t *proc, void *opaque) {
	struct match_state *st = opaque;
	if (!st) {
		return false;
	}

	size_t next_off = st->off;

	int rc = regex_exec_slice(st->prog, st->hay, st->hay_len, st->off, &st->m, &next_off);
	dprintf("regex exec slice, next_off=%lu\n", next_off);

	if (rc == RE_AGAIN) {
		st->off = next_off;
		return true;                  /* keep idling */
	}

	dprintf("regex exec slice, done, rc=%d\n", rc);

	/* terminal states */
	st->off = next_off;
	st->matched = (rc == RE_OK);
	st->done = 1;                    /* mark complete for the statement */
	return false;                    /* stop idling; scheduler will re-enter line */
}


void match_statement(struct basic_ctx *ctx)
{
	accept_or_return(MATCH, ctx);

	size_t res_len = 0;
	const char *res_name = tokenizer_variable_name(ctx, &res_len);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);

	const char *pat = str_expr(ctx);
	size_t pat_len = strlen(pat);
	accept_or_return(COMMA, ctx);

	const char *hay = str_expr(ctx);
	size_t hay_len = strlen(hay);

	process_t *proc = proc_cur(logical_cpu_id());

	if (ctx->match_ctx) {
		struct match_state *st = ctx->match_ctx;
		int matched = st->matched;

		const char *emsg = regex_last_error(st->prog);
		if (emsg != NULL && emsg[0] != '\0') {
			tokenizer_error_printf(ctx, "MATCH: %s", emsg);
		}

		regex_free(st->prog);
		buddy_free(ctx->allocator, st);
		ctx->match_ctx = NULL;

		basic_set_int_variable(res_name, matched, ctx, false, false);
		accept_or_return(NEWLINE, ctx);
		proc->state = PROC_RUNNING;
		return;
	}


	struct regex_prog *prog = NULL;
	int rc = regex_compile(ctx->allocator, &prog, (const uint8_t *)pat, pat_len);
	if (rc != RE_OK) {
		const char *emsg = (prog != NULL) ? regex_last_error(prog) : "";
		if (emsg != NULL && emsg[0] != '\0') {
			tokenizer_error_printf(ctx, "MATCH: %s", emsg);
		} else {
			tokenizer_error_printf(ctx, "MATCH: invalid regular expression (%d)", rc);
		}
		if (prog != NULL) {
			regex_free(prog);
		}
		return;
	}

	struct match_state *st = buddy_malloc(ctx->allocator, sizeof(*st));
	if (!st) {
		regex_free(prog);
		tokenizer_error_printf(ctx, "MATCH: out of memory");
		return;
	}

	memset(st, 0, sizeof(*st));
	st->prog          = prog;
	st->hay           = (const uint8_t *)hay;
	st->hay_len       = hay_len;
	st->off           = 0;
	st->fuel_per_tick = 5000;

	ctx->match_ctx = st;

	proc_set_idle(proc, match_idle_step, st);
	jump_linenum(ctx->current_linenum, ctx);
	proc->state = PROC_IO_BOUND;
}

char* basic_chr(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("CHR$","");
	char res[2] = {(unsigned char)intval, 0};
	return gc_strdup(ctx, res);
}

char* basic_str(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("STR$","");
	char res[MAX_STRINGLEN];
	snprintf(res, MAX_STRINGLEN, "%ld", intval);
	return gc_strdup(ctx, res);
}

char* basic_bool(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("BOOL$","");
	char res[MAX_STRINGLEN];
	snprintf(res, MAX_STRINGLEN, "%s", intval ? "TRUE" : "FALSE");
	return gc_strdup(ctx, res);
}

int64_t basic_instr(struct basic_ctx* ctx)
{
	const char *haystack, *needle;

	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	haystack = strval ? strval : "";
	PARAMS_GET_ITEM(BIP_STRING);
	needle = strval ? strval : "";
	PARAMS_END("INSTR", 0);

	if (*needle == '\0') {
		return 1;
	}

	/* Quick length check avoids calling strstr on impossible matches */
	size_t hlen = strlen(haystack);
	size_t nlen = strlen(needle);
	if (nlen > hlen) {
		return 0;
	}

	const char *p = strstr(haystack, needle);
	if (!p) {
		return 0;
	}

	/* 1-based index as 0 represents substring not found */
	return (int64_t)((p - haystack) + 1);
}


char* basic_upper(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("UPPER$","");
	char* modified = gc_strdup(ctx, strval);
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
	char* modified = gc_strdup(ctx, strval);
	for (char* m = modified; *m; ++m) {
		*m = tolower(*m);
	}
	return modified;
}

char* basic_highlight(struct basic_ctx* ctx) {
	GENERATE_ENUM_STRING_NAMES(TOKEN, token_names)
	GENERATE_ENUM_STRING_LENGTHS(TOKEN, token_name_lengths)
	const size_t token_count = sizeof(token_names) / sizeof(*token_names);
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* in = strval;
	char out[MAX_STRINGLEN] = {};
	PARAMS_END("HIGHLIGHT$","");
	bool in_quotes = false, in_comment = false;
	const char* end = in + strlen(in);
	for (const char* pos = in; *pos; ++pos) {
		size_t current_len = strlen(out);
		bool found = false, reset_colour = false;
		if (in_comment && current_len < MAX_STRINGLEN - 1) {
			*(out + current_len) = *pos;
			continue;
		} else if (!in_quotes && *pos == '"') {
			current_len += snprintf(out + current_len, MAX_STRINGLEN - current_len, "\x1b[%um", map_vga_to_ansi(COLOUR_LIGHTYELLOW));
			in_quotes = true;
		} else if (in_quotes && *pos == '"') {
			in_quotes = false;
			reset_colour = true;
		} else if (!in_quotes && *pos == '\'') {
			in_comment = true;
			current_len += snprintf(out + current_len, MAX_STRINGLEN - current_len, "\x1b[%um", map_vga_to_ansi(COLOUR_DARKGREEN));
		}
		if (!in_quotes && !in_comment) {
			for (size_t v = 0; builtin_int[v].name; ++v) {
				size_t fn_len = strlen(builtin_int[v].name);
				if (pos + fn_len <= end && !memcmp(pos, builtin_int[v].name, fn_len)) {
					/* Is a builtin integer function */
					snprintf(out + current_len, MAX_STRINGLEN - current_len, "\x1b[%um%s\x1b[%um", map_vga_to_ansi(COLOUR_LIGHTGREEN), builtin_int[v].name, map_vga_to_ansi(COLOUR_WHITE));
					found = true;
					pos += fn_len - 1;
				}
			}
			for (size_t v = 0; builtin_str[v].name; ++v) {
				size_t fn_len = strlen(builtin_str[v].name);
				if (pos + fn_len <= end && !memcmp(pos, builtin_str[v].name, fn_len)) {
					/* Is a builtin string function */
					snprintf(out + current_len, MAX_STRINGLEN - current_len, "\x1b[%um%s\x1b[%um", map_vga_to_ansi(COLOUR_LIGHTMAGENTA), builtin_str[v].name, map_vga_to_ansi(COLOUR_WHITE));
					found = true;
					pos += fn_len - 1;
				}
			}
			for (size_t v = 0; builtin_double[v].name; ++v) {
				size_t fn_len = strlen(builtin_double[v].name);
				if (pos + fn_len <= end && !memcmp(pos, builtin_double[v].name, fn_len)) {
					/* Is a builtin real function */
					snprintf(out + current_len, MAX_STRINGLEN - current_len, "\x1b[%um%s\x1b[%um", map_vga_to_ansi(COLOUR_LIGHTCYAN), builtin_double[v].name, map_vga_to_ansi(COLOUR_WHITE));
					found = true;
					pos += fn_len - 1;
				}
			}
			for (size_t v = 0; v < token_count; ++v) {
				size_t kw_len = token_name_lengths[v];
				if (pos + kw_len <= end && !strncmp(pos, token_names[v], kw_len)) {
					const char *after = pos + kw_len;
					bool next_is_varlike = ((*after >= '0' && *after <= '9') || (toupper(*after) >= 'A' && toupper(*after) <= 'Z') || *after == '_');
					if (!next_is_varlike || v == PROC || v == FN || v == EQUALS) {
						/* Is a token */
						if (v == REM) {
							in_comment = true;
							snprintf(out, MAX_STRINGLEN, "\x1b[%um%s\x1b[%um", map_vga_to_ansi(COLOUR_DARKGREEN), in, map_vga_to_ansi(COLOUR_WHITE));
							return (char*)gc_strdup(ctx, out);
						} else {
							snprintf(out + current_len, MAX_STRINGLEN - current_len, "\x1b[%um%s\x1b[%um", map_vga_to_ansi(COLOUR_LIGHTBLUE), token_names[v], map_vga_to_ansi(COLOUR_WHITE));
							found = true;
						}
						pos += kw_len - 1;
						break;
					}
				}
			}
		}
		if (!found) {
			if (!in_quotes && !in_comment && ((*pos >= '0' && *pos <= '9') || ((*pos == '-' || *pos == '+') && (*(pos+1) >= '0' && *(pos+1) <= '9')))) {
				/* Numeric colour */
				snprintf(out + current_len, MAX_STRINGLEN - current_len, "\x1b[%um%c\x1b[%um", map_vga_to_ansi(COLOUR_ORANGE), *pos, map_vga_to_ansi(COLOUR_WHITE));
			} else if (!in_quotes && !in_comment && (*pos == '(' || *pos == ')' || *pos == '+' || *pos == '-' || *pos == '/' || *pos == '=' ||  *pos == '*' || *pos == '<' || *pos == '>' || *pos == ',' || *pos == ';')) {
				/* Symbolic maths colour */
				snprintf(out + current_len, MAX_STRINGLEN - current_len, "\x1b[%um%c\x1b[%um", map_vga_to_ansi(COLOUR_DARKRED), *pos, map_vga_to_ansi(COLOUR_WHITE));
			} else if (current_len < MAX_STRINGLEN - 1) {
				*(out + current_len) = *pos;
			}
		}
		if (reset_colour) {
			snprintf(out + current_len, MAX_STRINGLEN - current_len, "\"\x1b[%um", map_vga_to_ansi(COLOUR_WHITE));
		}
	}
	char buf[MAX_STRINGLEN];
	snprintf(buf, MAX_STRINGLEN, "%s\x1b[%um", out, map_vga_to_ansi(COLOUR_WHITE));
	return (char*)gc_strdup(ctx, buf);
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
			return gc_strdup(ctx, return_value);
		}
		current_value++;
		ofs++;
	}
	char return_value[MAX_STRINGLEN];
	strlcpy(return_value, old_value, MAX_STRINGLEN);
	basic_set_string_variable(varname, "", ctx, false, false);
	return gc_strdup(ctx, return_value);
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
	char* cut = gc_strdup(ctx, strval);
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
	return gc_strdup(ctx, strval + len - intval);
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
	char* cut = gc_strdup(ctx, strval);
	*(cut + end) = 0;
	return cut + start;
}

char* basic_replace(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* haystack = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* needle = strval;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* with = strval;
	PARAMS_END("REPLACE$","");

	if (!haystack || *haystack == '\0') {
		return "";
	}

	size_t needle_len = needle ? strlen(needle) : 0;
	size_t with_len = with ? strlen(with) : 0;

	if (needle_len == 0) {
		return (char*)gc_strdup(ctx, haystack);
	}

	char out[MAX_STRINGLEN];
	size_t w = 0;
	const char* p = haystack;

	while (*p && w < (MAX_STRINGLEN - 1)) {
		if (strncmp(p, needle, needle_len) == 0) {
			if (with_len != 0) {
				size_t space = (MAX_STRINGLEN - 1) - w;
				size_t ncopy = with_len <= space ? with_len : space;
				if (ncopy != 0) {
					memcpy(out + w, with, ncopy);
					w += ncopy;
				}
			}
			p += needle_len;
		} else {
			out[w++] = *p++;
		}
	}

	out[w] = '\0';
	return (char*)gc_strdup(ctx, out);
}

int64_t basic_len(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	PARAMS_END("LEN",0);
	return strlen(strval);
}

char* basic_ljust(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = strval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t width = intval;
	PARAMS_GET_ITEM(BIP_STRING);
	char* fill_string = strval;
	PARAMS_END("LJUST$", "");
	if (strlen(fill_string) < 1) {
		tokenizer_error_print(ctx, "No fill character");
		return "";
	}
	char fill_char = fill_string[0];
	int64_t target_length = strlen(target);
	int64_t result_length = width > target_length ? width : target_length;
	char mresult[result_length + 1];
	memcpy(mresult, target, target_length + 1);
	memset(mresult + target_length, fill_char, result_length - target_length);
	mresult[result_length] = '\0';
	return (char*)gc_strdup(ctx, mresult);
}

char* basic_rjust(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = strval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t width = intval;
	PARAMS_GET_ITEM(BIP_STRING);
	char* fill_string = strval;
	PARAMS_END("RJUST$", "");
	if (strlen(fill_string) < 1) {
		tokenizer_error_print(ctx, "No fill character");
		return "";
	}
	char fill_char = fill_string[0];
	int64_t target_length = strlen(target);
	if (width <= target_length) {
		return (char*)gc_strdup(ctx, target);
	}
	int64_t result_length = width > target_length ? width : target_length;
	char mresult[result_length + 1];
	memset(mresult, fill_char, result_length - target_length);
	memcpy(mresult + result_length - target_length, target, target_length);
	mresult[result_length] = '\0';
	return (char*)gc_strdup(ctx, mresult);
}

char* basic_ltrim(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = strval;
	PARAMS_END("LTRIM$", "");
	while (isspace(*target)) {
		++target;
	}
	return (char*)gc_strdup(ctx, target);
}

char* basic_rtrim(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = gc_strdup(ctx, strval);
	PARAMS_END("RTRIM$", "");
	int64_t lastIndex;
	while (isspace(target[lastIndex = strlen(target) - 1])) {
		target[lastIndex] = '\0';
	}
	return target;
}

char* basic_trim(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = gc_strdup(ctx, strval);
	PARAMS_END("TRIM$", "");
	while (isspace(*target)) {
		++target;
	}
	int64_t lastIndex;
	while (isspace(target[lastIndex = strlen(target) - 1])) {
		target[lastIndex] = '\0';
	}
	return target;
}

char* basic_itoa(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t target = intval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t radix = intval;
	PARAMS_END("RADIX$", "");
	char buffer[MAX_STRINGLEN] = {0};
	int err;
	if ((err = do_itoa(target, buffer, radix)) >= 0) {
		return (char*)gc_strdup(ctx, buffer);
	}
	switch (-err) {
	case 1:
		tokenizer_error_print(ctx, "Invalid radix (not in range between 2 and 36)");
		return "";
	}
	tokenizer_error_printf(ctx, "Unknown `do_itoa` error: %d", -err);
	return "";
}

char* basic_reverse(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = (char*)gc_strdup(ctx, strval);
	PARAMS_END("REVERSE$", "");
	strrev(target);
	return target;
}

char* basic_repeat(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = strval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t count = intval;
	PARAMS_END("REP$", "");
	if (count < 0) {
		return (char*)gc_strdup(ctx, "");
	}
	int64_t target_length = strlen(target);
	char tmp[target_length * count + 2];
	int64_t i;
	char* ptr = tmp;
	for (i = 0; i < count; ++i) {
		memcpy(ptr, target, target_length);
		ptr += target_length;
	}
	*ptr = '\0';
	return (char*)gc_strdup(ctx, tmp);
}
