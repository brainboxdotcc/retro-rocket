#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/*
 * Variable getter/setter functions
 */
int64_t basic_get_int_variable(const char* varname, struct basic_ctx* ctx);
bool basic_get_double_variable(const char* var, struct basic_ctx* ctx, double* res);
const char* basic_get_string_variable(const char* var, struct basic_ctx* ctx);
void basic_set_string_variable(const char* var, const char* value, struct basic_ctx* ctx, bool local, bool global);
void basic_set_double_variable(const char* var, const double value, struct basic_ctx* ctx, bool local, bool global);
void basic_set_int_variable(const char* var, int64_t value, struct basic_ctx* ctx, bool local, bool global);
ub_return_type basic_get_numeric_variable(const char* var, struct basic_ctx* ctx, double* res);
int64_t basic_get_numeric_int_variable(const char* var, struct basic_ctx* ctx);
void let_statement(struct basic_ctx* ctx, bool global, bool local);
bool basic_int_variable_exists(const char* var, struct basic_ctx* ctx);
bool basic_string_variable_exists(const char* var, struct basic_ctx* ctx);
bool basic_double_variable_exists(const char* var, struct basic_ctx* ctx);

