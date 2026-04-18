/**
 * @file basic/array.c
 * @brief BASIC console IO functions
 */
#include <kernel.h>

ub_var_int_array* find_int_array(const char* var, struct basic_ctx* ctx)
{
	if (!var) {
		return NULL;
	}
	return hashmap_get(ctx->int_array_variables, &(ub_var_int_array) { .varname = var });
}

ub_var_string_array* find_string_array(const char* var, struct basic_ctx* ctx)
{
	if (!var) {
		return NULL;
	}
	return hashmap_get(ctx->string_array_variables, &(ub_var_string_array) { .varname = var });
}

ub_var_double_array* find_double_array(const char* var, struct basic_ctx* ctx)
{
	if (!var) {
		return NULL;
	}
	return hashmap_get(ctx->double_array_variables, &(ub_var_double_array) { .varname = var });
}

bool varname_is_int_array_access(struct basic_ctx* ctx, const char* varname)
{
	return find_int_array(varname, ctx) != NULL;
}

bool varname_is_string_array_access(struct basic_ctx* ctx, const char* varname)
{
	return find_string_array(varname, ctx) != NULL;
}

bool varname_is_double_array_access(struct basic_ctx* ctx, const char* varname)
{
	return find_double_array(varname, ctx) != NULL;
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
	if (!var) {
		return "";
	}
	if (index < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", index);
		return "";
	}

	struct ub_var_string_array* cur = find_string_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return "";
	}

	if ((uint64_t)index >= cur->itemcount) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
		return "";
	}

	if (cur->values[index]) {
		return gc_strdup(ctx, cur->values[index]);
	}

	return "";
}

int64_t basic_get_int_array_variable(const char* var, int64_t index, struct basic_ctx* ctx)
{
	if (index < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", index);
		return 0;
	}

	struct ub_var_int_array* cur = find_int_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return 0;
	}

	if ((uint64_t)index >= cur->itemcount) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
		return 0;
	}

	return cur->values[index];
}

bool basic_get_double_array_variable(const char* var, int64_t index, struct basic_ctx* ctx, double* ret)
{
	if (index < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", index);
		*ret = 0;
		return false;
	}

	struct ub_var_double_array* cur = find_double_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		*ret = 0;
		return false;
	}

	if ((uint64_t)index >= cur->itemcount) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
		return false;
	}

	*ret = cur->values[index];
	return true;
}

static void init_int_array(struct basic_ctx* ctx, ub_var_int_array* array, const char* varname, size_t len, int64_t size)
{
	array->name_length = len;
	array->varname = buddy_strdup(ctx->allocator, varname);
	array->itemcount = size;
	array->values = buddy_malloc(ctx->allocator, sizeof(int64_t) * size);
}

static void init_string_array(struct basic_ctx* ctx, ub_var_string_array* array, const char* varname, size_t len, int64_t size)
{
	array->name_length = len;
	array->varname = buddy_strdup(ctx->allocator, varname);
	array->itemcount = size;
	array->values = buddy_malloc(ctx->allocator, sizeof(char*) * size);
}

static void init_double_array(struct basic_ctx* ctx, ub_var_double_array* array, const char* varname, size_t len, int64_t size)
{
	array->name_length = len;
	array->varname = buddy_strdup(ctx->allocator, varname);
	array->itemcount = size;
	array->values = buddy_malloc(ctx->allocator, sizeof(double) * size);
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
	if (find_int_array(var, ctx)) {
		tokenizer_error_printf(ctx, "Array '%s' already dimensioned", var);
		return false;
	}

	size_t len = strlen(var);
	ub_var_int_array new;
	init_int_array(ctx, &new, var, len, size);
	if (!new.varname || !new.values) {
		tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
		return false;
	}

	for (int64_t v = 0; v < size; ++v) {
		new.values[v] = 0;
	}

	if (!hashmap_set(ctx->int_array_variables, &new) && hashmap_oom(ctx->int_array_variables)) {
		tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
		return false;
	}

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
	if (find_string_array(var, ctx)) {
		tokenizer_error_printf(ctx, "Array '%s' already dimensioned", var);
		return false;
	}

	size_t len = strlen(var);
	ub_var_string_array new;
	init_string_array(ctx, &new, var, len, size);
	if (!new.varname || !new.values) {
		tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
		return false;
	}

	for (int64_t v = 0; v < size; ++v) {
		new.values[v] = NULL;
	}

	if (!hashmap_set(ctx->string_array_variables, &new) && hashmap_oom(ctx->string_array_variables)) {
		tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
		return false;
	}

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
	if (find_double_array(var, ctx)) {
		tokenizer_error_printf(ctx, "Array '%s' already dimensioned", var);
		return false;
	}

	size_t len = strlen(var);
	ub_var_double_array new;
	init_double_array(ctx, &new, var, len, size);
	if (!new.varname || !new.values) {
		tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
		return false;
	}

	for (int64_t v = 0; v < size; ++v) {
		new.values[v] = 0.0;
	}

	if (!hashmap_set(ctx->double_array_variables, &new) && hashmap_oom(ctx->double_array_variables)) {
		tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
		return false;
	}

	return true;
}

bool basic_redim_int_array(const char* var, int64_t size, struct basic_ctx* ctx)
{
	if (size < 1) {
		tokenizer_error_printf(ctx, "Invalid array size %ld", size);
		return false;
	}

	struct ub_var_int_array* cur = find_int_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return false;
	}

	if ((uint64_t)size == cur->itemcount) {
		return true;
	}

	int64_t* new_values = buddy_realloc(ctx->allocator, cur->values, sizeof(int64_t) * size);
	if (!new_values) {
		tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
		return false;
	}
	cur->values = new_values;

	if ((uint64_t)size > cur->itemcount) {
		for (int64_t i = cur->itemcount; i < size; ++i) {
			cur->values[i] = 0;
		}
	}

	cur->itemcount = size;
	return true;
}

bool basic_redim_string_array(const char* var, int64_t size, struct basic_ctx* ctx)
{
	if (size < 1) {
		tokenizer_error_printf(ctx, "Invalid array size %ld", size);
		return false;
	}

	struct ub_var_string_array* cur = find_string_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return false;
	}

	if ((uint64_t)size == cur->itemcount) {
		return true;
	}

	if ((uint64_t)size < cur->itemcount) {
		for (uint64_t x = size; x < (uint64_t)cur->itemcount; ++x) {
			buddy_free(ctx->allocator, cur->values[x]);
		}
	}

	const char** new_values = buddy_realloc(ctx->allocator, cur->values, sizeof(char*) * size);
	if (!new_values) {
		tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
		return false;
	}
	cur->values = new_values;

	if ((uint64_t)size > cur->itemcount) {
		for (int64_t i = cur->itemcount; i < size; ++i) {
			cur->values[i] = NULL;
		}
	}

	cur->itemcount = size;
	return true;
}

bool basic_redim_double_array(const char* var, int64_t size, struct basic_ctx* ctx)
{
	if (size < 1) {
		tokenizer_error_printf(ctx, "Invalid array size %ld", size);
		return false;
	}

	struct ub_var_double_array* cur = find_double_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return false;
	}

	if ((uint64_t)size == cur->itemcount) {
		return true;
	}

	double* new_values = buddy_realloc(ctx->allocator, cur->values, sizeof(double) * size);
	if (!new_values) {
		tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
		return false;
	}
	cur->values = new_values;

	if ((uint64_t)size > cur->itemcount) {
		for (int64_t i = cur->itemcount; i < size; ++i) {
			cur->values[i] = 0;
		}
	}

	cur->itemcount = size;
	return true;
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

	struct ub_var_string_array* cur = find_string_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return;
	}

	if ((uint64_t)index >= cur->itemcount) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
		return;
	}

	char* newval = buddy_strdup(ctx->allocator, value);
	if (!newval) {
		tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
		return;
	}

	buddy_free(ctx->allocator, cur->values[index]);
	cur->values[index] = newval;
}

void basic_set_string_array(const char* var, const char* value, struct basic_ctx* ctx)
{
	if (!valid_string_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}

	struct ub_var_string_array* cur = find_string_array(var, ctx);
	if (!cur) {
		return;
	}

	for (uint64_t x = 0; x < cur->itemcount; ++x) {
		char* newval = buddy_strdup(ctx->allocator, value);
		if (!newval) {
			tokenizer_error_printf(ctx, "Array '%s': Out of memory", var);
			return;
		}
		buddy_free(ctx->allocator, cur->values[x]);
		cur->values[x] = newval;
	}
}

void basic_set_int_array(const char* var, int64_t value, struct basic_ctx* ctx)
{
	if (!valid_int_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}

	struct ub_var_int_array* cur = find_int_array(var, ctx);
	if (!cur) {
		return;
	}

	for (uint64_t x = 0; x < cur->itemcount; ++x) {
		cur->values[x] = value;
	}
}

void basic_set_double_array(const char* var, double value, struct basic_ctx* ctx)
{
	if (!valid_double_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}

	struct ub_var_double_array* cur = find_double_array(var, ctx);
	if (!cur) {
		return;
	}

	for (uint64_t x = 0; x < cur->itemcount; ++x) {
		cur->values[x] = value;
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

	struct ub_var_double_array* cur = find_double_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return;
	}

	if ((uint64_t)index >= cur->itemcount) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
		return;
	}

	cur->values[index] = value;
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

	struct ub_var_int_array* cur = find_int_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return;
	}

	if ((uint64_t)index >= cur->itemcount) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", index, cur->itemcount - 1);
		return;
	}

	cur->values[index] = value;
}

void dim_statement(struct basic_ctx* ctx)
{
	accept_or_return(DIM, ctx);
	size_t var_length;
	const char* array_name = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t array_size = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	char last = array_name[var_length - 1];
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
	size_t var_length;
	const char* array_name = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t array_size = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	char last = array_name[var_length - 1];
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

int64_t arr_target_index(struct basic_ctx* ctx)
{
	while (*ctx->ptr != '(' && *ctx->ptr != '\n' && *ctx->ptr) {
		ctx->ptr++;
	}

	if (*ctx->ptr != '(') {
		accept(VARIABLE, ctx);
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
	return index;
}

int64_t arr_expr_set_index(struct basic_ctx* ctx, [[maybe_unused]] const char* varname)
{
	int64_t index = arr_target_index(ctx);
	accept(EQUALS, ctx);
	return index;
}

bool basic_pop_string_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx)
{
	if (pop_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", pop_pos);
		return false;
	}

	struct ub_var_string_array* cur = find_string_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return false;
	}

	if ((uint64_t)pop_pos >= cur->itemcount) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", pop_pos, cur->itemcount - 1);
		return false;
	}

	buddy_free(ctx->allocator, cur->values[pop_pos]);
	for (uint64_t i = (uint64_t)pop_pos; i < cur->itemcount - 1; ++i) {
		cur->values[i] = cur->values[i + 1];
	}
	cur->values[cur->itemcount - 1] = NULL;
	return true;
}

bool basic_pop_int_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx)
{
	if (pop_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", pop_pos);
		return false;
	}

	struct ub_var_int_array* cur = find_int_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return false;
	}

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

bool basic_pop_double_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx)
{
	if (pop_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", pop_pos);
		return false;
	}

	struct ub_var_double_array* cur = find_double_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return false;
	}

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

bool basic_push_string_array(const char* var, int64_t push_pos, struct basic_ctx* ctx)
{
	if (push_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", push_pos);
		return false;
	}

	struct ub_var_string_array* cur = find_string_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return false;
	}

	if ((uint64_t)push_pos >= cur->itemcount) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds [0..%ld]", push_pos, cur->itemcount - 1);
		return false;
	}
	if (cur->itemcount < 2) {
		tokenizer_error_printf(ctx, "Array too small for PUSH [0..%ld]", cur->itemcount - 1);
		return false;
	}
	if (cur->values[cur->itemcount - 1]) {
		buddy_free(ctx->allocator, cur->values[cur->itemcount - 1]);
	}
	for (int64_t i = (int64_t)cur->itemcount - 2; i >= push_pos; --i) {
		cur->values[i + 1] = cur->values[i];
	}
	cur->values[push_pos] = NULL;
	return true;
}

bool basic_push_int_array(const char* var, int64_t push_pos, struct basic_ctx* ctx)
{
	if (push_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", push_pos);
		return false;
	}

	struct ub_var_int_array* cur = find_int_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return false;
	}

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

bool basic_push_double_array(const char* var, int64_t push_pos, struct basic_ctx* ctx)
{
	if (push_pos < 0) {
		tokenizer_error_printf(ctx, "Array index %ld out of bounds", push_pos);
		return false;
	}

	struct ub_var_double_array* cur = find_double_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return false;
	}

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

void push_statement(struct basic_ctx* ctx)
{
	accept_or_return(PUSH, ctx);
	size_t var_length;
	const char* array_name = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t push_pos = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	char last = array_name[var_length - 1];
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
	size_t var_length;
	const char* array_name = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);
	int64_t pop_pos = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	char last = array_name[var_length - 1];
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

static bool ensure_int_result_array(const char* varname, int64_t size, struct basic_ctx* ctx)
{
	if (varname_is_string_array_access(ctx, varname) || varname_is_double_array_access(ctx, varname)) {
		tokenizer_error_printf(ctx, "Variable '%s' already exists as non-integer array type", varname);
		return false;
	}

	if (!varname_is_int_array_access(ctx, varname)) {
		return basic_dim_int_array(varname, size, ctx);
	}

	return basic_redim_int_array(varname, size, ctx);
}

static bool basic_arrayfind_int(const char* source, int64_t needle, const char* dest, const char* count_var, struct basic_ctx* ctx)
{
	struct ub_var_int_array* cur = find_int_array(source, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", source);
		return false;
	}

	int64_t matches = 0;

	for (uint64_t i = 0; i < cur->itemcount; ++i) {
		if (cur->values[i] == needle) {
			matches++;
		}
	}

	basic_set_int_variable(count_var, matches, ctx, false, false);
	if (ctx->errored) {
		return false;
	}

	if (matches < 1) {
		if (!ensure_int_result_array(dest, 1, ctx)) {
			return false;
		}
		basic_set_int_array_variable(dest, 0, -1, ctx);
		return !ctx->errored;
	}

	if (!ensure_int_result_array(dest, matches, ctx)) {
		return false;
	}

	int64_t out = 0;

	for (uint64_t i = 0; i < cur->itemcount; ++i) {
		if (cur->values[i] == needle) {
			basic_set_int_array_variable(dest, out, i, ctx);
			if (ctx->errored) {
				return false;
			}
			out++;
		}
	}

	return true;
}

static bool basic_arrayfind_double(const char* source, double needle, const char* dest, const char* count_var, struct basic_ctx* ctx)
{
	struct ub_var_double_array* cur = find_double_array(source, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", source);
		return false;
	}

	int64_t matches = 0;

	for (uint64_t i = 0; i < cur->itemcount; ++i) {
		if (cur->values[i] == needle) {
			matches++;
		}
	}

	basic_set_int_variable(count_var, matches, ctx, false, false);
	if (ctx->errored) {
		return false;
	}

	if (matches < 1) {
		if (!ensure_int_result_array(dest, 1, ctx)) {
			return false;
		}
		basic_set_int_array_variable(dest, 0, -1, ctx);
		return !ctx->errored;
	}

	if (!ensure_int_result_array(dest, matches, ctx)) {
		return false;
	}

	int64_t out = 0;

	for (uint64_t i = 0; i < cur->itemcount; ++i) {
		if (cur->values[i] == needle) {
			basic_set_int_array_variable(dest, out, i, ctx);
			if (ctx->errored) {
				return false;
			}
			out++;
		}
	}

	return true;
}

static bool basic_arrayfind_string(const char* source, const char* needle, const char* dest, const char* count_var, struct basic_ctx* ctx)
{
	struct ub_var_string_array* cur = find_string_array(source, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", source);
		return false;
	}

	int64_t matches = 0;

	for (uint64_t i = 0; i < cur->itemcount; ++i) {
		const char* value = cur->values[i] ? cur->values[i] : "";
		if (!strcmp(value, needle)) {
			matches++;
		}
	}

	basic_set_int_variable(count_var, matches, ctx, false, false);
	if (ctx->errored) {
		return false;
	}

	if (matches < 1) {
		if (!ensure_int_result_array(dest, 1, ctx)) {
			return false;
		}
		basic_set_int_array_variable(dest, 0, -1, ctx);
		return !ctx->errored;
	}

	if (!ensure_int_result_array(dest, matches, ctx)) {
		return false;
	}

	int64_t out = 0;

	for (uint64_t i = 0; i < cur->itemcount; ++i) {
		const char* value = cur->values[i] ? cur->values[i] : "";
		if (!strcmp(value, needle)) {
			basic_set_int_array_variable(dest, out, i, ctx);
			if (ctx->errored) {
				return false;
			}
			out++;
		}
	}

	return true;
}

void arrayfind_statement(struct basic_ctx* ctx)
{
	accept_or_return(ARRAYFIND, ctx);

	size_t src_length;
	const char* source = tokenizer_variable_name(ctx, &src_length);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);

	if (varname_is_int_array_access(ctx, source)) {
		int64_t needle = expr(ctx);
		accept_or_return(COMMA, ctx);

		size_t dest_length;
		const char* dest = tokenizer_variable_name(ctx, &dest_length);
		accept_or_return(VARIABLE, ctx);
		accept_or_return(COMMA, ctx);

		size_t count_length;
		const char* count_var = tokenizer_variable_name(ctx, &count_length);
		accept_or_return(VARIABLE, ctx);
		accept_or_return(NEWLINE, ctx);

		if (!strcmp(source, dest)) {
			tokenizer_error_print(ctx, "Source and destination arrays must differ");
			return;
		}

		if (!strcmp(source, count_var) || !strcmp(dest, count_var)) {
			tokenizer_error_print(ctx, "Count variable must differ from source and destination");
			return;
		}

		basic_arrayfind_int(source, needle, dest, count_var, ctx);
		return;
	}

	if (varname_is_double_array_access(ctx, source)) {
		double needle = 0;
		double_expr(ctx, &needle);

		accept_or_return(COMMA, ctx);

		size_t dest_length;
		const char* dest = tokenizer_variable_name(ctx, &dest_length);
		accept_or_return(VARIABLE, ctx);
		accept_or_return(COMMA, ctx);

		size_t count_length;
		const char* count_var = tokenizer_variable_name(ctx, &count_length);
		accept_or_return(VARIABLE, ctx);
		accept_or_return(NEWLINE, ctx);

		if (!strcmp(source, dest)) {
			tokenizer_error_print(ctx, "Source and destination arrays must differ");
			return;
		}

		if (!strcmp(source, count_var) || !strcmp(dest, count_var)) {
			tokenizer_error_print(ctx, "Count variable must differ from source and destination");
			return;
		}

		basic_arrayfind_double(source, needle, dest, count_var, ctx);
		return;
	}

	if (varname_is_string_array_access(ctx, source)) {
		const char* needle = str_expr(ctx);
		accept_or_return(COMMA, ctx);

		size_t dest_length;
		const char* dest = tokenizer_variable_name(ctx, &dest_length);
		accept_or_return(VARIABLE, ctx);
		accept_or_return(COMMA, ctx);

		size_t count_length;
		const char* count_var = tokenizer_variable_name(ctx, &count_length);
		accept_or_return(VARIABLE, ctx);
		accept_or_return(NEWLINE, ctx);

		if (!strcmp(source, dest)) {
			tokenizer_error_print(ctx, "Source and destination arrays must differ");
			return;
		}

		if (!strcmp(source, count_var) || !strcmp(dest, count_var)) {
			tokenizer_error_print(ctx, "Count variable must differ from source and destination");
			return;
		}

		basic_arrayfind_string(source, needle, dest, count_var, ctx);
		return;
	}

	tokenizer_error_printf(ctx, "No such array variable '%s'", source);
}

static int compare_int_array_asc(const void* a, const void* b)
{
	int64_t left = *(const int64_t*)a;
	int64_t right = *(const int64_t*)b;

	if (left < right) {
		return -1;
	}
	if (left > right) {
		return 1;
	}

	return 0;
}

static int compare_int_array_desc(const void* a, const void* b)
{
	int64_t left = *(const int64_t*)a;
	int64_t right = *(const int64_t*)b;

	if (left > right) {
		return -1;
	}
	if (left < right) {
		return 1;
	}

	return 0;
}

static int compare_double_array_asc(const void* a, const void* b)
{
	double left = *(const double*)a;
	double right = *(const double*)b;

	if (left < right) {
		return -1;
	}
	if (left > right) {
		return 1;
	}

	return 0;
}

static int compare_double_array_desc(const void* a, const void* b)
{
	double left = *(const double*)a;
	double right = *(const double*)b;

	if (left > right) {
		return -1;
	}
	if (left < right) {
		return 1;
	}

	return 0;
}

static int compare_string_array_asc(const void* a, const void* b)
{
	const char* left = *(char* const*)a;
	const char* right = *(char* const*)b;

	if (!left) {
		left = "";
	}
	if (!right) {
		right = "";
	}

	return strcmp(left, right);
}

static int compare_string_array_desc(const void* a, const void* b)
{
	const char* left = *(char* const*)a;
	const char* right = *(char* const*)b;

	if (!left) {
		left = "";
	}
	if (!right) {
		right = "";
	}

	return strcmp(right, left);
}

static void basic_sort_int_array(const char* var, bool descending, struct basic_ctx* ctx)
{
	struct ub_var_int_array* cur = find_int_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return;
	}

	if (cur->itemcount < 2) {
		return;
	}

	qsort(
		cur->values,
		cur->itemcount,
		sizeof(int64_t),
		descending ? compare_int_array_desc : compare_int_array_asc
	);
}

static void basic_sort_double_array(const char* var, bool descending, struct basic_ctx* ctx)
{
	struct ub_var_double_array* cur = find_double_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return;
	}

	if (cur->itemcount < 2) {
		return;
	}

	qsort(
		cur->values,
		cur->itemcount,
		sizeof(double),
		descending ? compare_double_array_desc : compare_double_array_asc
	);
}

static void basic_sort_string_array(const char* var, bool descending, struct basic_ctx* ctx)
{
	struct ub_var_string_array* cur = find_string_array(var, ctx);
	if (!cur) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", var);
		return;
	}

	if (cur->itemcount < 2) {
		return;
	}

	qsort(
		cur->values,
		cur->itemcount,
		sizeof(char*),
		descending ? compare_string_array_desc : compare_string_array_asc
	);
}

void arrsort_statement(struct basic_ctx* ctx)
{
	accept_or_return(ARRSORT, ctx);

	size_t var_length;
	const char* array_name = tokenizer_variable_name(ctx, &var_length);
	accept_or_return(VARIABLE, ctx);

	bool descending = false;

	if (ctx->current_token == COMMA) {
		accept_or_return(COMMA, ctx);
		descending = expr(ctx) != 0;
	}

	accept_or_return(NEWLINE, ctx);

	char last = array_name[var_length - 1];
	switch (last) {
		case '#':
			basic_sort_double_array(array_name, descending, ctx);
			break;
		case '$':
			basic_sort_string_array(array_name, descending, ctx);
			break;
		default:
			basic_sort_int_array(array_name, descending, ctx);
			break;
	}
}

static void swap_int64(int64_t* a, int64_t* b)
{
	int64_t tmp = *a;
	*a = *b;
	*b = tmp;
}

static bool int_key_out_of_order(int64_t left_index, int64_t right_index, const int64_t* keys, bool descending)
{
	int64_t left_key = keys[left_index];
	int64_t right_key = keys[right_index];

	if (descending) {
		return left_key < right_key;
	}

	return left_key > right_key;
}

static bool double_key_out_of_order(int64_t left_index, int64_t right_index, const double* keys, bool descending)
{
	double left_key = keys[left_index];
	double right_key = keys[right_index];

	if (descending) {
		return left_key < right_key;
	}

	return left_key > right_key;
}

static bool string_key_out_of_order(int64_t left_index, int64_t right_index, const char* const* keys, bool descending)
{
	const char* left_key = keys[left_index] ? keys[left_index] : "";
	const char* right_key = keys[right_index] ? keys[right_index] : "";
	int cmp = strcmp(left_key, right_key);

	if (descending) {
		return cmp < 0;
	}

	return cmp > 0;
}

static void quicksort_indices_by_string_keys(int64_t* values, int64_t low, int64_t high, const char* const* keys, bool descending)
{
	while (low < high) {
		int64_t i = low;
		int64_t j = high;
		int64_t pivot_index = values[low + (high - low) / 2];

		for (;;) {
			while (string_key_out_of_order(pivot_index, values[i], keys, descending)) {
				i++;
			}

			while (string_key_out_of_order(values[j], pivot_index, keys, descending)) {
				j--;
			}

			if (i >= j) {
				break;
			}

			swap_int64(&values[i], &values[j]);
			i++;
			j--;
		}

		if (j - low < high - j) {
			if (low < j) {
				quicksort_indices_by_string_keys(values, low, j, keys, descending);
			}
			low = j + 1;
		} else {
			if (j + 1 < high) {
				quicksort_indices_by_string_keys(values, j + 1, high, keys, descending);
			}
			high = j;
		}
	}
}

static void quicksort_indices_by_int_keys(int64_t* values, int64_t low, int64_t high, const int64_t* keys, bool descending)
{
	while (low < high) {
		int64_t i = low;
		int64_t j = high;
		int64_t pivot_index = values[low + (high - low) / 2];

		for (;;) {
			while (int_key_out_of_order(pivot_index, values[i], keys, descending)) {
				i++;
			}

			while (int_key_out_of_order(values[j], pivot_index, keys, descending)) {
				j--;
			}

			if (i >= j) {
				break;
			}

			swap_int64(&values[i], &values[j]);
			i++;
			j--;
		}

		if (j - low < high - j) {
			if (low < j) {
				quicksort_indices_by_int_keys(values, low, j, keys, descending);
			}
			low = j + 1;
		} else {
			if (j + 1 < high) {
				quicksort_indices_by_int_keys(values, j + 1, high, keys, descending);
			}
			high = j;
		}
	}
}

static void quicksort_indices_by_double_keys(int64_t* values, int64_t low, int64_t high, const double* keys, bool descending)
{
	while (low < high) {
		int64_t i = low;
		int64_t j = high;
		int64_t pivot_index = values[low + (high - low) / 2];

		for (;;) {
			while (double_key_out_of_order(pivot_index, values[i], keys, descending)) {
				i++;
			}

			while (double_key_out_of_order(values[j], pivot_index, keys, descending)) {
				j--;
			}

			if (i >= j) {
				break;
			}

			swap_int64(&values[i], &values[j]);
			i++;
			j--;
		}

		if (j - low < high - j) {
			if (low < j) {
				quicksort_indices_by_double_keys(values, low, j, keys, descending);
			}
			low = j + 1;
		} else {
			if (j + 1 < high) {
				quicksort_indices_by_double_keys(values, j + 1, high, keys, descending);
			}
			high = j;
		}
	}
}

static bool basic_arrsortby_int_keys(const char* index_var, const char* key_var, bool descending, struct basic_ctx* ctx)
{
	struct ub_var_int_array* indices = find_int_array(index_var, ctx);
	if (!indices) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", index_var);
		return false;
	}

	struct ub_var_int_array* keys = find_int_array(key_var, ctx);
	if (!keys) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", key_var);
		return false;
	}

	if (indices->itemcount != keys->itemcount) {
		tokenizer_error_printf(ctx, "Index array '%s' and key array '%s' differ in size", index_var, key_var);
		return false;
	}

	for (uint64_t i = 0; i < indices->itemcount; ++i) {
		if (indices->values[i] < 0 || (uint64_t)indices->values[i] >= keys->itemcount) {
			tokenizer_error_printf(ctx, "Index value %ld out of bounds for key array '%s' [0..%ld]", indices->values[i], key_var, keys->itemcount - 1);
			return false;
		}
	}

	if (indices->itemcount < 2) {
		return true;
	}

	quicksort_indices_by_int_keys(indices->values, 0, (int64_t)indices->itemcount - 1, keys->values, descending);
	return true;
}

static bool basic_arrsortby_double_keys(const char* index_var, const char* key_var, bool descending, struct basic_ctx* ctx)
{
	struct ub_var_int_array* indices = find_int_array(index_var, ctx);
	if (!indices) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", index_var);
		return false;
	}

	struct ub_var_double_array* keys = find_double_array(key_var, ctx);
	if (!keys) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", key_var);
		return false;
	}

	if (indices->itemcount != keys->itemcount) {
		tokenizer_error_printf(ctx, "Index array '%s' and key array '%s' differ in size", index_var, key_var);
		return false;
	}

	for (uint64_t i = 0; i < indices->itemcount; ++i) {
		if (indices->values[i] < 0 || (uint64_t)indices->values[i] >= keys->itemcount) {
			tokenizer_error_printf(ctx, "Index value %ld out of bounds for key array '%s' [0..%ld]", indices->values[i], key_var, keys->itemcount - 1);
			return false;
		}
	}

	if (indices->itemcount < 2) {
		return true;
	}

	quicksort_indices_by_double_keys(indices->values, 0, (int64_t)indices->itemcount - 1, keys->values, descending);
	return true;
}

static bool basic_arrsortby_string_keys(const char* index_var, const char* key_var, bool descending, struct basic_ctx* ctx)
{
	struct ub_var_int_array* indices = find_int_array(index_var, ctx);
	if (!indices) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", index_var);
		return false;
	}

	struct ub_var_string_array* keys = find_string_array(key_var, ctx);
	if (!keys) {
		tokenizer_error_printf(ctx, "No such array variable '%s'", key_var);
		return false;
	}

	if (indices->itemcount != keys->itemcount) {
		tokenizer_error_printf(ctx, "Index array '%s' and key array '%s' differ in size", index_var, key_var);
		return false;
	}

	for (uint64_t i = 0; i < indices->itemcount; ++i) {
		if (indices->values[i] < 0 || (uint64_t)indices->values[i] >= keys->itemcount) {
			tokenizer_error_printf(ctx, "Index value %ld out of bounds for key array '%s' [0..%ld]", indices->values[i], key_var, keys->itemcount - 1);
			return false;
		}
	}

	if (indices->itemcount < 2) {
		return true;
	}

	quicksort_indices_by_string_keys(indices->values, 0, (int64_t)indices->itemcount - 1, keys->values, descending);
	return true;
}

void arrsortby_statement(struct basic_ctx* ctx)
{
	accept_or_return(ARRSORTBY, ctx);

	size_t index_length;
	const char* index_var = tokenizer_variable_name(ctx, &index_length);
	accept_or_return(VARIABLE, ctx);
	accept_or_return(COMMA, ctx);

	size_t key_length;
	const char* key_var = tokenizer_variable_name(ctx, &key_length);
	accept_or_return(VARIABLE, ctx);

	bool descending = false;

	if (ctx->current_token == COMMA) {
		accept_or_return(COMMA, ctx);
		descending = expr(ctx) != 0;
	}

	accept_or_return(NEWLINE, ctx);

	if (!varname_is_int_array_access(ctx, index_var)) {
		tokenizer_error_printf(ctx, "Index array '%s' must be an integer array", index_var);
		return;
	}

	if (!strcmp(index_var, key_var)) {
		tokenizer_error_print(ctx, "Index and key arrays must differ");
		return;
	}

	if (varname_is_int_array_access(ctx, key_var)) {
		basic_arrsortby_int_keys(index_var, key_var, descending, ctx);
		return;
	}

	if (varname_is_double_array_access(ctx, key_var)) {
		basic_arrsortby_double_keys(index_var, key_var, descending, ctx);
		return;
	}

	if (varname_is_string_array_access(ctx, key_var)) {
		basic_arrsortby_string_keys(index_var, key_var, descending, ctx);
		return;
	}

	tokenizer_error_printf(ctx, "No such array variable '%s'", key_var);
}