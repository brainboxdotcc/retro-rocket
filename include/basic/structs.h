#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief State for a FOR...NEXT loop
 */
typedef struct for_state {
	int64_t line_after_for;
	const char* for_variable;
	int64_t to;
	int64_t step;
} for_state;

/**
 * @brief An integer variable
 */
typedef struct ub_var_int {
	const char* varname;
	size_t name_length;
	int64_t value;
	bool global;
	struct ub_var_int* next;
} ub_var_int;

/**
 * @brief A real (double) variable
 */
typedef struct ub_var_double {
	const char* varname;
	size_t name_length;
	double value;
	bool global;
	struct ub_var_double* next;
} ub_var_double;

/**
 * @brief A string variable
 */
typedef struct ub_var_string {
	const char* varname;
	size_t name_length;
	char* value;
	bool global;
	struct ub_var_string* next;
} ub_var_string;

/**
 * @brief Function or procedure
 */
typedef enum ub_fn_type {
	FT_FN,
	FT_PROC
} ub_fn_type;

/**
 * @brief Return type of FN, PROC
 */
typedef enum ub_return_type {
	RT_MAIN,	// In main
	RT_NONE,	// No return value
	RT_STRING,	// String return value
	RT_INT,		// Integer return value
	RT_FLOAT,	// Double return value
} ub_return_type;

/**
 * @brief FN/PROC parameter
 */
typedef struct ub_param {
	const char* name;
	struct ub_param* next;
} ub_param;

/**
 * @brief Procedure or function definition
 * Each has a name, a type, a starting line and a parameter list
 */
typedef struct ub_proc_fn_def {
	const char* name;
	ub_fn_type type;
	int64_t line;
	struct ub_param* params;
	struct ub_proc_fn_def* next;
} ub_proc_fn_def;

/**
 * @brief An array of integers
 */
typedef struct ub_var_int_array {
	uint64_t itemcount;
	const char* varname;
	int64_t* values;
	struct ub_var_int_array* next;
} ub_var_int_array;

/**
 * @brief An array of strings
 */
typedef struct ub_var_string_array {
	uint64_t itemcount;
	const char* varname;
	const char** values;
	struct ub_var_string_array* next;
} ub_var_string_array;

/**
 * @brief An array of real (double)
 */
typedef struct ub_var_double_array {
	uint64_t itemcount;
	const char* varname;
	double* values;
	struct ub_var_double_array* next;
} ub_var_double_array;

/**
 * @brief A generic array, we can use
 * this to represent any array regardless
 * of its contained type.
 */
typedef struct ub_var_generic_array {
	uint64_t itemcount;
	const char* varname;
	void* values_inaccesible;
	struct ub_var_generic_array* next;
} ub_var_generic_array;

/**
 * @brief Line reference in program.
 * Each program has a hashmap of these so it can find a line number
 * in O(1) time.
 */
typedef struct ub_line_ref {
	uint32_t line_number;
	const char* ptr;
} ub_line_ref;

/**
 * @brief CPUID instruction result.
 */
typedef struct cpuid_result {
	unsigned int eax;
	unsigned int ebx;
	unsigned int ecx;
	unsigned int edx;
} cpuid_result_t;

typedef struct g_cpuid_vendor {
	char const* varname;
	char const* vendor;
} g_cpuid_vendor_t;

typedef struct sprite_t {
	uint32_t width;
	uint32_t height;
	uint32_t* pixels;
} sprite_t;

/**
 * @brief Integer function signature
 */
typedef int64_t (*builtin_int_fn)(struct basic_ctx* ctx);

/**
 * @brief String function signature
 */
typedef char* (*builtin_str_fn)(struct basic_ctx* ctx);

/**
 * @brief Real (double) function signature
 */
typedef void (*builtin_double_fn)(struct basic_ctx* ctx, double* res);

/**
 * @brief Builtin integer function
 */
typedef struct basic_int_fn
{
	builtin_int_fn handler;
	const char* name;
} basic_int_fn;

/**
 * @brief Builtin real (double) function
 */
typedef struct basic_double_fn
{
	builtin_double_fn handler;
	const char* name;
} basic_double_fn;

/**
 * @brief Builtin string function
 */
typedef struct basic_str_fn
{
	builtin_str_fn handler;
	const char* name;
} basic_str_fn;

typedef enum parameter_type_t {
	BIP_STRING,
	BIP_INT,
	BIP_DOUBLE,
	BIP_VARIABLE,
} parameter_type_t;
