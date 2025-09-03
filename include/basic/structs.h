#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/**
 * @brief State for a FOR...NEXT loop
 *
 * This structure stores the necessary state to manage a FOR loop, including
 * the line number to jump to after the loop ends, the loop variable, the "TO"
 * value, and the "STEP" value for incrementing the loop variable.
 */
typedef struct for_state {
	int64_t line_after_for; ///< Line number to jump to after the FOR loop
	const char* for_variable; ///< The loop variable
	int64_t to; ///< The "TO" value for the loop
	int64_t step; ///< The "STEP" value for the loop
} for_state;

/**
 * @brief An integer variable
 *
 * This structure represents an integer variable in the BASIC interpreter,
 * with its name, value, and whether it is global or local. It also supports
 * chaining for handling multiple variables.
 */
typedef struct ub_var_int {
	const char* varname; ///< Name of the integer variable
	size_t name_length; ///< Length of the variable name
	int64_t value; ///< The value of the integer variable
	bool global; ///< True if the variable is global, false if local
	struct ub_var_int* next; ///< Pointer to the next integer variable (for chaining)
} ub_var_int;

/**
 * @brief A real (double) variable
 *
 * This structure represents a real (double) variable in the BASIC interpreter,
 * with its name, value, and whether it is global or local. It also supports
 * chaining for handling multiple variables.
 */
typedef struct ub_var_double {
	const char* varname; ///< Name of the double variable
	size_t name_length; ///< Length of the variable name
	double value; ///< The value of the double variable
	bool global; ///< True if the variable is global, false if local
	struct ub_var_double* next; ///< Pointer to the next double variable (for chaining)
} ub_var_double;

/**
 * @brief A string variable
 *
 * This structure represents a string variable in the BASIC interpreter,
 * with its name, value, and whether it is global or local. It also supports
 * chaining for handling multiple variables.
 */
typedef struct ub_var_string {
	const char* varname; ///< Name of the string variable
	size_t name_length; ///< Length of the variable name
	char* value; ///< The value of the string variable
	bool global; ///< True if the variable is global, false if local
} ub_var_string;

/**
 * @brief Function or procedure type
 *
 * An enumeration that differentiates between a function (FN) and a procedure (PROC).
 */
typedef enum ub_fn_type {
	FT_FN, ///< Function type
	FT_PROC ///< Procedure type
} ub_fn_type;

/**
 * @brief Return type for FN, PROC
 *
 * An enumeration to specify the expected return type of functions or procedures.
 */
typedef enum ub_return_type {
	RT_MAIN, ///< Main program (no return value)
	RT_NONE, ///< No return value
	RT_STRING, ///< String return value
	RT_INT, ///< Integer return value
	RT_FLOAT, ///< Double return value
} ub_return_type;

/**
 * @brief FN/PROC parameter
 *
 * This structure represents a single parameter for a function or procedure,
 * containing the name of the parameter and a pointer to the next parameter.
 */
typedef struct ub_param {
	const char* name; ///< Name of the parameter
	struct ub_param* next; ///< Pointer to the next parameter
} ub_param;

/**
 * @brief Procedure or function definition
 *
 * This structure holds information about a function or procedure, including
 * its name, type (FN/PROC), starting line number, and a linked list of parameters.
 */
typedef struct ub_proc_fn_def {
	const char* name; ///< Name of the function or procedure
	ub_fn_type type; ///< Type of function or procedure (FN/PROC)
	int64_t line; ///< Starting line number of the function or procedure
	struct ub_param* params; ///< Linked list of function parameters
	struct ub_proc_fn_def* next; ///< Pointer to the next function or procedure definition
} ub_proc_fn_def;

/**
 * @brief An array of integers
 *
 * This structure represents an array of integers in the BASIC interpreter,
 * with a name, the number of items in the array, and the array values. It also
 * supports chaining for handling multiple arrays.
 */
typedef struct ub_var_int_array {
	uint64_t itemcount; ///< Number of items in the array
	const char* varname; ///< Name of the integer array variable
	int64_t* values; ///< Array of integer values
	struct ub_var_int_array* next; ///< Pointer to the next integer array (for chaining)
} ub_var_int_array;

/**
 * @brief An array of strings
 *
 * This structure represents an array of strings in the BASIC interpreter,
 * with a name, the number of items in the array, and the array values. It also
 * supports chaining for handling multiple arrays.
 */
typedef struct ub_var_string_array {
	uint64_t itemcount; ///< Number of items in the array
	const char* varname; ///< Name of the string array variable
	const char** values; ///< Array of string values
	struct ub_var_string_array* next; ///< Pointer to the next string array (for chaining)
} ub_var_string_array;

/**
 * @brief An array of real (double) values
 *
 * This structure represents an array of real (double) values in the BASIC interpreter,
 * with a name, the number of items in the array, and the array values. It also
 * supports chaining for handling multiple arrays.
 */
typedef struct ub_var_double_array {
	uint64_t itemcount; ///< Number of items in the array
	const char* varname; ///< Name of the double array variable
	double* values; ///< Array of double values
	struct ub_var_double_array* next; ///< Pointer to the next double array (for chaining)
} ub_var_double_array;

/**
 * @brief A generic array, we can use this to represent any array regardless
 * of its contained type.
 *
 * This structure represents a generic array in the BASIC interpreter,
 * where the type of values stored is not specified, allowing flexibility
 * in handling various types of arrays.
 */
typedef struct ub_var_generic_array {
	uint64_t itemcount; ///< Number of items in the array
	const char* varname; ///< Name of the generic array variable
	void* values_inaccesible; ///< A generic pointer to the array values
	struct ub_var_generic_array* next; ///< Pointer to the next generic array (for chaining)
} ub_var_generic_array;

/**
 * @brief Line reference in program
 *
 * This structure represents a line number reference in the program, used
 * for quick lookup in the hashmap of lines, allowing O(1) time complexity
 * when searching for line numbers.
 */
typedef struct ub_line_ref {
	uint32_t line_number; ///< Line number in the program
	const char* ptr; ///< Pointer to the start of the line in the program text
} ub_line_ref;

/**
 * @brief CPUID instruction result
 *
 * This structure holds the result of a CPUID instruction, which provides
 * detailed information about the CPU, such as supported features and capabilities.
 */
typedef struct cpuid_result {
	unsigned int eax; ///< The EAX register value from the CPUID instruction
	unsigned int ebx; ///< The EBX register value from the CPUID instruction
	unsigned int ecx; ///< The ECX register value from the CPUID instruction
	unsigned int edx; ///< The EDX register value from the CPUID instruction
} cpuid_result_t;

/**
 * @brief CPUID vendor information
 *
 * This structure holds the vendor information for the CPUID instruction.
 */
typedef struct g_cpuid_vendor {
	char const* varname; ///< Name of the variable holding the vendor information
	char const* vendor; ///< Vendor string obtained from the CPUID instruction
} g_cpuid_vendor_t;

/**
 * @brief Sprite data structure
 *
 * This structure represents a sprite, which is an image or object used
 * in graphics rendering. It contains the width, height, and pixel data
 * for the sprite image.
 */
typedef struct sprite_t {
	uint32_t width; ///< Width of the sprite in pixels
	uint32_t height; ///< Height of the sprite in pixels
	uint32_t* pixels; ///< Pointer to the pixel data of the sprite
} sprite_t;

/**
 * @brief Integer function signature
 *
 * This is a function pointer type for builtin functions that return an integer.
 */
typedef int64_t (*builtin_int_fn)(struct basic_ctx* ctx);

/**
 * @brief String function signature
 *
 * This is a function pointer type for builtin functions that return a string.
 */
typedef char* (*builtin_str_fn)(struct basic_ctx* ctx);

/**
 * @brief Real (double) function signature
 *
 * This is a function pointer type for builtin functions that return a double.
 */
typedef void (*builtin_double_fn)(struct basic_ctx* ctx, double* res);

/**
 * @brief Builtin integer function
 *
 * This structure represents a builtin integer function, with a handler
 * function pointer and the function's name.
 */
typedef struct basic_int_fn
{
	builtin_int_fn handler; ///< Function pointer for the handler
	const char* name; ///< Name of the builtin integer function
} basic_int_fn;

/**
 * @brief Builtin real (double) function
 *
 * This structure represents a builtin real (double) function, with a handler
 * function pointer and the function's name.
 */
typedef struct basic_double_fn
{
	builtin_double_fn handler; ///< Function pointer for the handler
	const char* name; ///< Name of the builtin double function
} basic_double_fn;

/**
 * @brief Builtin string function
 *
 * This structure represents a builtin string function, with a handler
 * function pointer and the function's name.
 */
typedef struct basic_str_fn
{
	builtin_str_fn handler; ///< Function pointer for the handler
	const char* name; ///< Name of the builtin string function
} basic_str_fn;

/**
 * @brief Parameter type enumeration
 *
 * This enumeration defines the types of function parameters that can
 * be passed, such as string, integer, double, or variable.
 */
typedef enum parameter_type_t {
	BIP_STRING, ///< String parameter
	BIP_INT, ///< Integer parameter
	BIP_DOUBLE, ///< Double parameter
	BIP_VARIABLE, ///< Variable parameter
} parameter_type_t;
