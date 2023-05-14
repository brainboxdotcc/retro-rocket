/**
 * @file basic/string.c
 * @brief BASIC string manipulation functions
 */
#include <kernel.h>

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

char* basic_ljust(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = strval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t width = intval;
	PARAMS_GET_ITEM(BIP_STRING);
	char* fillString = strval;
	PARAMS_END("LJUST$", "");
	if (strlen(fillString) < 1) {
		tokenizer_error_print(ctx, "No fill character");
		return "";
	}
	char fillChar = fillString[0];
	int64_t targetLength = strlen(target);
	int64_t resultLength = width > targetLength ? width : targetLength;
	char* mresult = kmalloc(resultLength);
	memcpy(mresult, target, targetLength + 1);
	memset(mresult + targetLength, fillChar, resultLength - targetLength);
	mresult[resultLength] = '\0';
	char* result = gc_strdup(mresult);
	kfree(mresult);
	return result;
}

char* basic_rjust(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = strval;
	PARAMS_GET_ITEM(BIP_INT);
	int64_t width = intval;
	PARAMS_GET_ITEM(BIP_STRING);
	char* fillString = strval;
	PARAMS_END("RJUST$", "");
	if (strlen(fillString) < 1) {
		tokenizer_error_print(ctx, "No fill character");
		return "";
	}
	char fillChar = fillString[0];
	int64_t targetLength = strlen(target);
	if (width <= targetLength) {
		return gc_strdup(target);
	}
	int64_t resultLength = width > targetLength ? width : targetLength;
	char* mresult = kmalloc(resultLength);
	memset(mresult, fillChar, resultLength - targetLength);
	memcpy(mresult + resultLength - targetLength, target, targetLength);
	mresult[resultLength] = '\0';
	char* result = gc_strdup(mresult);
	kfree(mresult);
	return result;
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
	return gc_strdup(target);
}

char* basic_rtrim(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = gc_strdup(strval);
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
	char* target = gc_strdup(strval);
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
	char buffer[128] = {0};
	int err;
	if ((err = do_itoa(target, buffer, radix)) >= 0) {
		return gc_strdup(buffer);
	}
	switch (-err) {
	case 1:
		tokenizer_error_print(ctx, "Invalid radix (not in range between 2 and 36)");
		return "";
	}
	sprintf(buffer, "Unknown `do_itoa` error: %d", -err);
	tokenizer_error_print(ctx, buffer);
	return "";
}

char* basic_reverse(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	char* target = gc_strdup(strval);
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
	PARAMS_END("REPEAT$", "");
	if (count < 0) {
		return gc_strdup("");
	}
	int64_t targetLength = strlen(target);
	char* tmp = kmalloc(targetLength * count + 1);
	int64_t i;
	char* ptr = tmp;
	for (i = 0; i < count; ++i) {
		memcpy(ptr, target, targetLength);
		ptr += targetLength;
	}
	*ptr = '\0';
	char* res = gc_strdup(tmp);
	kfree(tmp);
	return res;
}
