/**
 * @file basic/array.c
 * @brief BASIC console IO functions
 */
#include <kernel.h>

bool varname_is_int_array_access(struct basic_ctx* ctx, const char* varname)
{
	for (ub_var_int_array* i = ctx->int_array_variables; i; i = i->next) {
		if (!strcmp(i->varname, varname)) {
			return true;
		}
	}
	return false;
}

bool varname_is_string_array_access(struct basic_ctx* ctx, const char* varname)
{
	for (ub_var_string_array* i = ctx->string_array_variables; i; i = i->next) {
		if (!strcmp(i->varname, varname)) {
			return true;
		}
	}
	return false;
}

bool varname_is_double_array_access(struct basic_ctx* ctx, const char* varname)
{
	for (ub_var_double_array* i = ctx->double_array_variables; i; i = i->next) {
		if (!strcmp(i->varname, varname)) {
			return true;
		}
	}
	return false;
}

int64_t arr_variable_index(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("array subscript", 0)
	return intval;
}

const char* basic_get_string_array_variable(const char* var, int64_t index, struct basic_ctx* ctx)
{
	if (index < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", index);
		return "";
	}
	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
				return "";
			}
			if (cur->values[index]) {
				return gc_strdup(cur->values[index]);
			}
			return "";
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return "";
}

int64_t basic_get_int_array_variable(const char* var, int64_t index, struct basic_ctx* ctx)
{
	if (index < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", index);
		return 0;
	}
	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
				return 0;
			}
			return cur->values[index];
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return 0;
}

bool basic_get_double_array_variable(const char* var, int64_t index, struct basic_ctx* ctx, double* ret)
{
	if (index < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", index);
		*ret = 0;
		return false;
	}
	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
				return false;
			}
			*ret = cur->values[index];
			return true;
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	*ret = 0;
	return false;
}

bool basic_dim_int_array(const char* var, int64_t size, struct basic_ctx* ctx)
{
	if (basic_int_variable_exists(var, ctx)) {
		tokenizer_error_printf(ctx, "Variable '%s' already exists as non-array type", var);
		return false;
	}
	if (!valid_int_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return false;
	}
	if (size < 1) {
		tokenizer_error_printf(ctx, "Invalid array size %ld", size);
		return false;
	}

	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			tokenizer_error_printf(ctx, "Array '%s' already dimensioned", var);
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

bool basic_dim_string_array(const char* var, int64_t size, struct basic_ctx* ctx)
{
	if (basic_string_variable_exists(var, ctx)) {
		tokenizer_error_printf(ctx, "Variable '%s' already exists as non-array type", var);
		return false;
	}
	if (!valid_string_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return false;
	}
	if (size < 1) {
		tokenizer_error_printf(ctx, "Invalid array size %ld", size);
		return false;
	}

	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			tokenizer_error_printf(ctx, "Array '%s' already dimensioned", var);
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

bool basic_dim_double_array(const char* var, int64_t size, struct basic_ctx* ctx)
{
	if (basic_double_variable_exists(var, ctx)) {
		tokenizer_error_printf(ctx, "Variable '%s' already exists as non-array type", var);
		return false;
	}
	if (!valid_double_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return false;
	}
	if (size < 1) {
		tokenizer_error_printf(ctx, "Invalid array size %ld", size);
		return false;
	}

	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			tokenizer_error_printf(ctx, "Array '%s' already dimensioned", var);
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

bool basic_redim_int_array(const char* var, int64_t size, struct basic_ctx* ctx)
{
	if (size < 1) {
		tokenizer_error_printf(ctx, "Invalid array size %ld", size);
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
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return false;
}

bool basic_redim_string_array(const char* var, int64_t size, struct basic_ctx* ctx)
{
	if (size < 1) {
		tokenizer_error_printf(ctx, "Invalid array size %ld", size);
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
					kfree_null(&cur->values[x]);
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
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return false;	
}

bool basic_redim_double_array(const char* var, int64_t size, struct basic_ctx* ctx)
{
	if (size < 1) {
		tokenizer_error_printf(ctx, "Invalid array size %ld", size);
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
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return false;	
}

void basic_set_string_array_variable(const char* var, int64_t index, const char* value, struct basic_ctx* ctx)
{
	if (!valid_string_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}
	if (index < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", index);
		return;
	}

	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
				return;
			}
			kfree_null(&cur->values[index]);
			cur->values[index] = strdup(value);
			return;
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
}

void basic_set_string_array(const char* var, const char* value, struct basic_ctx* ctx)
{
	if (!valid_string_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}
	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			for (uint64_t x = 0; x < cur->itemcount; ++x) {
				kfree_null(&cur->values[x]);
				cur->values[x] = strdup(value);
			}
		}
	}
}

void basic_set_int_array(const char* var, int64_t value, struct basic_ctx* ctx)
{
	if (!valid_int_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
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

void basic_set_double_array(const char* var, double value, struct basic_ctx* ctx)
{
	if (!valid_double_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
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

void basic_set_double_array_variable(const char* var, int64_t index, double value, struct basic_ctx* ctx)
{
	if (!valid_double_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}
	if (index < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", index);
		return;
	}

	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
				return;
			}
			cur->values[index] = value;
			return;
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
}

void basic_set_int_array_variable(const char* var, int64_t index, int64_t value, struct basic_ctx* ctx)
{
	if (!valid_int_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}
	if (index < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", index);
		return;
	}

	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)index >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
				return;
			}
			cur->values[index] = value;
			return;
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
}


void dim_statement(struct basic_ctx* ctx)
{
	accept_or_return(DIM, ctx);
	const char* array_name = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t array_size = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	char last = array_name[strlen(array_name) - 1];
	switch (last) {
		case '#':
			basic_dim_double_array(array_name, array_size, ctx);
		break;
		case '$':
			basic_dim_string_array(array_name, array_size, ctx);
		break;
		default:
			basic_dim_int_array(array_name, array_size, ctx);
	}
}

void redim_statement(struct basic_ctx* ctx)
{
	accept_or_return(REDIM, ctx);
	const char* array_name = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t array_size = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	char last = array_name[strlen(array_name) - 1];
	switch (last) {
		case '#':
			basic_redim_double_array(array_name, array_size, ctx);
		break;
		case '$':
			basic_redim_string_array(array_name, array_size, ctx);
		break;
		default:
			basic_redim_int_array(array_name, array_size, ctx);
	}
}

int64_t arr_expr_set_index(struct basic_ctx* ctx, const char* varname)
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
		tokenizer_error_print(ctx, "Array subscripts cannot be negative");
		return 0;
	}
	int64_t index = expr(ctx);
	accept(CLOSEBRACKET, ctx);
	accept(EQUALS, ctx);
	return index;
}

bool basic_pop_string_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx)
{
	if (pop_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", pop_pos);
		return false;
	}
	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)pop_pos >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", pop_pos, cur->itemcount - 1);
				return false;
			}
			kfree_null(&cur->values[pop_pos]);
			for (uint64_t i = (uint64_t)pop_pos; i < cur->itemcount - 1; ++i) {
				cur->values[i] = cur->values[i + 1];
			}
			cur->values[cur->itemcount - 1] = NULL;
			return true;
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return false;	
}

bool basic_pop_int_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx)
{
	if (pop_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", pop_pos);
		return false;
	}
	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)pop_pos >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", pop_pos, cur->itemcount - 1);
				return false;
			}
			for (uint64_t i = (uint64_t)pop_pos; i < cur->itemcount - 1; ++i) {
				cur->values[i] = cur->values[i + 1];
			}
			cur->values[cur->itemcount - 1] = 0;
			return true;
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return false;	
}

bool basic_pop_double_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx)
{
	if (pop_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", pop_pos);
		return false;
	}
	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)pop_pos >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", pop_pos, cur->itemcount - 1);
				return false;
			}
			for (uint64_t i = (uint64_t)pop_pos; i < cur->itemcount - 1; ++i) {
				cur->values[i] = cur->values[i + 1];
			}
			cur->values[cur->itemcount - 1] = 0;
			return true;
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return false;	
}

bool basic_push_string_array(const char* var, int64_t push_pos, struct basic_ctx* ctx)
{
	if (push_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", push_pos);
		return false;
	}
	struct ub_var_string_array* cur = ctx->string_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)push_pos >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", push_pos, cur->itemcount - 1);
				return false;
			}
			if (cur->itemcount < 2) {
				tokenizer_error_printf(ctx, "Array too small for PUSH [0..%ld]", cur->itemcount - 1);
				return false;
			}
			if (cur->values[cur->itemcount - 1]) {
				kfree_null(&cur->values[cur->itemcount - 1]);
			}
			for (int64_t i = (int64_t)cur->itemcount - 2; i >= push_pos; --i) {
				cur->values[i + 1] = cur->values[i];
			
			}
			cur->values[push_pos] = NULL;
			return true;
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return false;	
}

bool basic_push_int_array(const char* var, int64_t push_pos, struct basic_ctx* ctx)
{
	if (push_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", push_pos);
		return false;
	}
	struct ub_var_int_array* cur = ctx->int_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)push_pos >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", push_pos, cur->itemcount - 1);
				return false;
			}
			if (cur->itemcount < 2) {
				tokenizer_error_printf(ctx, "Array too small for PUSH [0..%ld]", cur->itemcount - 1);
				return false;
			}
			for (int64_t i = (int64_t)cur->itemcount - 2; i >= push_pos; --i) {
				cur->values[i + 1] = cur->values[i];
			}
			cur->values[push_pos] = 0;
			return true;
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return false;	
}

bool basic_push_double_array(const char* var, int64_t push_pos, struct basic_ctx* ctx)
{
	if (push_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", push_pos);
		return false;
	}
	struct ub_var_double_array* cur = ctx->double_array_variables;
	for (; cur; cur = cur->next) {
		if (!strcmp(var, cur->varname)) {
			if ((uint64_t)push_pos >= cur->itemcount) {
				tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", push_pos, cur->itemcount - 1);
				return false;
			}
			if (cur->itemcount < 2) {
				tokenizer_error_printf(ctx, "Array too small for PUSH [0..%ld]", cur->itemcount - 1);
				return false;
			}
			for (int64_t i = (int64_t)cur->itemcount - 2; i >= push_pos; --i) {
				cur->values[i + 1] = cur->values[i];
			}
			cur->values[push_pos] = 0;
			return true;
		}
	}
	tokenizer_error_printf(ctx, "No such array variable '%s'", var);
	return false;	
}

void push_statement(struct basic_ctx* ctx)
{
	accept_or_return(PUSH, ctx);
	const char* array_name = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t push_pos = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	char last = array_name[strlen(array_name) - 1];
	switch (last) {
		case '#':
			basic_push_double_array(array_name, push_pos, ctx);
		break;
		case '$':
			basic_push_string_array(array_name, push_pos, ctx);
		break;
		default:
			basic_push_int_array(array_name, push_pos, ctx);
	}
}

void pop_statement(struct basic_ctx* ctx)
{
	accept_or_return(POP, ctx);
	const char* array_name = tokenizer_variable_name(ctx);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t pop_pos = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	char last = array_name[strlen(array_name) - 1];
	switch (last) {
		case '#':
			basic_pop_double_array(array_name, pop_pos, ctx);
		break;
		case '$':
			basic_pop_string_array(array_name, pop_pos, ctx);
		break;
		default:
			basic_pop_int_array(array_name, pop_pos, ctx);
	}
}

