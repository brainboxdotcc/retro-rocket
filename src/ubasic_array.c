#include <kernel.h>

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
	PARAMS_END("array subscript")
	return intval;
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


void dim_statement(struct ubasic_ctx* ctx)
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

void redim_statement(struct ubasic_ctx* ctx)
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

int64_t arr_expr_set_index(struct ubasic_ctx* ctx, const char* varname)
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

bool ubasic_pop_string_array(const char* var, int64_t pop_pos, struct ubasic_ctx* ctx)
{
	if (pop_pos < 0) {
		tokenizer_error_print(ctx, "Invalid array index");
		return false;
	}
	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)pop_pos >= cur->itemcount) {
				tokenizer_error_print(ctx, "Invalid array index");
				return false;
			}
			if (cur->values[pop_pos]) {
				kfree(cur->values[pop_pos]);
			}
			for (uint64_t i = (uint64_t)pop_pos; i < cur->itemcount + 1; ++i) {
				cur->values[i] = cur->values[i + 1];
			}
			return true;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return false;	
}

bool ubasic_pop_int_array(const char* var, int64_t pop_pos, struct ubasic_ctx* ctx)
{
	if (pop_pos < 0) {
		tokenizer_error_print(ctx, "Invalid array index");
		return false;
	}
	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)pop_pos >= cur->itemcount) {
				tokenizer_error_print(ctx, "Invalid array index");
				return false;
			}
			for (uint64_t i = (uint64_t)pop_pos; i < cur->itemcount + 1; ++i) {
				cur->values[i] = cur->values[i + 1];
			}
			return true;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return false;	
}

bool ubasic_pop_double_array(const char* var, int64_t pop_pos, struct ubasic_ctx* ctx)
{
	if (pop_pos < 0) {
		tokenizer_error_print(ctx, "Invalid array index");
		return false;
	}
	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)pop_pos >= cur->itemcount) {
				tokenizer_error_print(ctx, "Invalid array index");
				return false;
			}
			for (uint64_t i = (uint64_t)pop_pos; i < cur->itemcount + 1; ++i) {
				cur->values[i] = cur->values[i + 1];
			}
			return true;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return false;	
}

bool ubasic_push_string_array(const char* var, int64_t push_pos, struct ubasic_ctx* ctx)
{
	if (push_pos < 0) {
		tokenizer_error_print(ctx, "Invalid array index");
		return false;
	}
	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)push_pos >= cur->itemcount) {
				tokenizer_error_print(ctx, "Invalid array index");
				return false;
			}
			if (cur->itemcount < 2) {
				tokenizer_error_print(ctx, "Array too small for PUSH");
				return false;
			}
			kfree(cur->values[cur->itemcount - 1]);
			cur->values[cur->itemcount - 1] = NULL;
			for (uint64_t i = cur->itemcount - 2; i >= (uint64_t)push_pos; --i) {
				cur->values[i + 1] = cur->values[i];
			}
			return true;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return false;	
}

bool ubasic_push_int_array(const char* var, int64_t push_pos, struct ubasic_ctx* ctx)
{
	if (push_pos < 0) {
		tokenizer_error_print(ctx, "Invalid array index");
		return false;
	}
	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)push_pos >= cur->itemcount) {
				tokenizer_error_print(ctx, "Invalid array index");
				return false;
			}
			if (cur->itemcount < 2) {
				tokenizer_error_print(ctx, "Array too small for PUSH");
				return false;
			}
			for (uint64_t i = cur->itemcount - 2; i >= (uint64_t)push_pos; --i) {
				cur->values[i + 1] = cur->values[i];
			}
			return true;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return false;	
}

bool ubasic_push_double_array(const char* var, int64_t push_pos, struct ubasic_ctx* ctx)
{
	if (push_pos < 0) {
		tokenizer_error_print(ctx, "Invalid array index");
		return false;
	}
	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)push_pos >= cur->itemcount) {
				tokenizer_error_print(ctx, "Invalid array index");
				return false;
			}
			if (cur->itemcount < 2) {
				tokenizer_error_print(ctx, "Array too small for PUSH");
				return false;
			}
			for (uint64_t i = cur->itemcount - 2; i >= (uint64_t)push_pos; --i) {
				cur->values[i + 1] = cur->values[i];
			}
			return true;
		}
	}
	tokenizer_error_print(ctx, "No such array variable");
	return false;	
}

void push_statement(struct ubasic_ctx* ctx)
{
	accept(PUSH, ctx);
	const char* array_name = tokenizer_variable_name(ctx);
	accept(VARIABLE, ctx);
	accept(COMMA, ctx);
	int64_t push_pos = expr(ctx);
	accept(NEWLINE, ctx);
	char last = array_name[strlen(array_name) - 1];
	kprintf("var name: '%s' push at '%d'\n", array_name, push_pos);
	switch (last) {
		case '#':
			ubasic_push_double_array(array_name, push_pos, ctx);
		break;
		case '$':
			ubasic_push_string_array(array_name, push_pos, ctx);
		break;
		default:
			ubasic_push_int_array(array_name, push_pos, ctx);
	}
}

void pop_statement(struct ubasic_ctx* ctx)
{
	accept(POP, ctx);
	const char* array_name = tokenizer_variable_name(ctx);
	accept(VARIABLE, ctx);
	accept(COMMA, ctx);
	int64_t pop_pos = expr(ctx);
	accept(NEWLINE, ctx);
	char last = array_name[strlen(array_name) - 1];
	switch (last) {
		case '#':
			ubasic_pop_double_array(array_name, pop_pos, ctx);
		break;
		case '$':
			ubasic_pop_string_array(array_name, pop_pos, ctx);
		break;
		default:
			ubasic_pop_int_array(array_name, pop_pos, ctx);
	}
}

