#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/*
 * Array manipulation functions
 */
void basic_set_int_array_variable(const char* var, int64_t index, int64_t value, struct basic_ctx* ctx);
void basic_set_string_array_variable(const char* var, int64_t index, const char* value, struct basic_ctx* ctx);
void basic_set_double_array_variable(const char* var, int64_t index, double value, struct basic_ctx* ctx);
bool basic_get_double_array_variable(const char* var, int64_t index, struct basic_ctx* ctx, double* ret);
int64_t basic_get_int_array_variable(const char* var, int64_t index, struct basic_ctx* ctx);
const char* basic_get_string_array_variable(const char* var, int64_t index, struct basic_ctx* ctx);
bool basic_dim_string_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool basic_dim_int_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool basic_dim_double_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool basic_redim_string_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool basic_redim_int_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool basic_redim_double_array(const char* var, int64_t size, struct basic_ctx* ctx);
bool varname_is_int_array_access(struct basic_ctx* ctx, const char* varname);
bool varname_is_string_array_access(struct basic_ctx* ctx, const char* varname);
bool varname_is_double_array_access(struct basic_ctx* ctx, const char* varname);
int64_t arr_variable_index(struct basic_ctx* ctx);
void basic_set_int_array(const char* var, int64_t value, struct basic_ctx* ctx);
void basic_set_string_array(const char* var, const char* value, struct basic_ctx* ctx);
void basic_set_double_array(const char* var, double value, struct basic_ctx* ctx);
void dim_statement(struct basic_ctx* ctx);
void redim_statement(struct basic_ctx* ctx);
void pop_statement(struct basic_ctx* ctx);
void push_statement(struct basic_ctx* ctx);
int64_t arr_expr_set_index(struct basic_ctx* ctx, const char* varname);
bool basic_pop_string_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx);
bool basic_pop_int_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx);
bool basic_pop_double_array(const char* var, int64_t pop_pos, struct basic_ctx* ctx);
bool basic_push_string_array(const char* var, int64_t push_pos, struct basic_ctx* ctx);
bool basic_push_int_array(const char* var, int64_t push_pos, struct basic_ctx* ctx);
bool basic_push_double_array(const char* var, int64_t push_pos, struct basic_ctx* ctx);

