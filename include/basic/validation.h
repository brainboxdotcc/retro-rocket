#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/*
 * Validation functions
 */
bool valid_int_var(const char* name);
bool valid_string_var(const char* name);
bool valid_double_var(const char* name);