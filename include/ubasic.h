/*
 * BBC BASIC interpreter,
 * Retro Rocket OS Project (C) Craig Edwards 2012.
 * loosely based on uBASIC (Copyright (c) 2006, Adam Dunkels, All rights reserved).
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#pragma once 

#define MAX_STRINGLEN 1024
#define MAX_GOSUB_STACK_DEPTH 255
#define MAX_FOR_STACK_DEPTH 50

typedef struct for_state {
        int64_t line_after_for;
        char* for_variable;
        int64_t to;
        int64_t step;
} for_state;

typedef struct ub_var_int {
	char* varname;
	int64_t value;
	bool global;
	struct ub_var_int* next;
} ub_var_int;

typedef struct ub_var_double {
	char* varname;
	double value;
	bool global;
	struct ub_var_double* next;
} ub_var_double;

typedef struct ub_var_string {
	char* varname; /* Not including the $ on the end! */
	char* value;
	bool global;
	struct ub_var_string* next;
} ub_var_string;

typedef enum ub_fn_type {
	FT_FN,
	FT_PROC
} ub_fn_type;

typedef enum ub_return_type {
	RT_STRING,
	RT_INT,
	RT_FLOAT,
} ub_return_type;

typedef struct ub_param {
	char* name;
	struct ub_param* next;
} ub_param;

typedef struct ub_proc_fn_def {
	char* name;
	ub_fn_type type;
	int64_t line;
	struct ub_param* params;
	struct ub_proc_fn_def* next;
} ub_proc_fn_def;

typedef struct ub_var_int_array {
	char* varname;
	struct ub_var_int* values;
	uint64_t itemcount;
} ub_var_int_array;

typedef struct ub_var_string_array {
	char* varname;
	struct ub_var_string* values;
	uint64_t itemcount;
} ub_var_string_array;

typedef struct ub_var_double_array {
	char* varname;
	struct ub_var_double* values;
	uint64_t itemcount;
} ub_var_double_array;

typedef struct ub_line_ref {
	uint32_t line_number;
	const char* ptr;
	struct ub_line_ref* prev;
	struct ub_line_ref* next;
} ub_line_ref;

typedef struct ubasic_ctx
{
        char const *ptr;
	char const* nextptr;
        int current_token;
	int64_t current_linenum;
	int errored;
        char *program_ptr;
        char string[MAX_STRINGLEN];
        uint64_t gosub_stack[MAX_GOSUB_STACK_DEPTH];
	struct ub_var_int* local_int_variables[MAX_GOSUB_STACK_DEPTH];
	struct ub_var_string* local_string_variables[MAX_GOSUB_STACK_DEPTH];
	struct ub_var_double* local_double_variables[MAX_GOSUB_STACK_DEPTH];
        uint64_t gosub_stack_ptr;
	int oldlen;
	int64_t eval_linenum;
        struct for_state for_stack[MAX_FOR_STACK_DEPTH];
        uint64_t for_stack_ptr;
	struct ub_proc_fn_def* defs;
        struct ub_var_int* int_variables;
	struct ub_var_string* str_variables;
	struct ub_var_double* double_variables;
	struct ub_var_int_array* int_array_variables;
	struct ub_var_string_array* string_array_variables;
	struct console* cons;
        int ended;
	ub_return_type fn_type;
	void* fn_return;
	int bracket_depth;
	char const* item_begin;
	struct ub_param* param;
	int32_t graphics_colour;	// Current GCOL
	ub_line_ref* lines;		// Doubly linked list of line numbers to char pointers
	ub_line_ref* line_tail;		// Pointer to last element of line list
} ubasic_ctx;


typedef int64_t (*builtin_int_fn)(struct ubasic_ctx* ctx);
typedef char* (*builtin_str_fn)(struct ubasic_ctx* ctx);
typedef void (*builtin_double_fn)(struct ubasic_ctx* ctx, double* res);

typedef struct ubasic_int_fn
{
	builtin_int_fn handler;
	const char* name;
} ubasic_int_fn;

typedef struct ubasic_double_fn
{
	builtin_double_fn handler;
	const char* name;
} ubasic_double_fn;

typedef struct ubasic_str_fn
{
	builtin_str_fn handler;
	const char* name;
} ubasic_str_fn;

// Builtin integer functions
int64_t ubasic_abs(struct ubasic_ctx* ctx);
int64_t ubasic_len(struct ubasic_ctx* ctx);
int64_t ubasic_openin(struct ubasic_ctx* ctx);
int64_t ubasic_openout(struct ubasic_ctx* ctx);
int64_t ubasic_openup(struct ubasic_ctx* ctx);
int64_t ubasic_eof(struct ubasic_ctx* ctx);
int64_t ubasic_read(struct ubasic_ctx* ctx);
int64_t ubasic_instr(struct ubasic_ctx* ctx);
int64_t ubasic_asc(struct ubasic_ctx* ctx);
int64_t ubasic_getnamecount(struct ubasic_ctx* ctx);
int64_t ubasic_getsize(struct ubasic_ctx* ctx);
int64_t ubasic_get_text_max_x(struct ubasic_ctx* ctx);
int64_t ubasic_get_text_max_y(struct ubasic_ctx* ctx);

// Builtin string functions
char* ubasic_netinfo(struct ubasic_ctx* ctx);
char* ubasic_left(struct ubasic_ctx* ctx);
char* ubasic_mid(struct ubasic_ctx* ctx);
char* ubasic_chr(struct ubasic_ctx* ctx);
char* ubasic_readstring(struct ubasic_ctx* ctx);
char* ubasic_getname(struct ubasic_ctx* ctx);

struct ubasic_ctx* ubasic_init(const char *program, console* cons, uint32_t pid, const char* file, char** error);
void ubasic_destroy(struct ubasic_ctx* ctx);
void ubasic_run(struct ubasic_ctx* ctx);
int ubasic_finished(struct ubasic_ctx* ctx);
int64_t ubasic_get_int_variable(const char* varname, struct ubasic_ctx* ctx);
bool ubasic_get_double_variable(const char* var, struct ubasic_ctx* ctx, double* res);
const char* ubasic_get_string_variable(const char* var, struct ubasic_ctx* ctx);
void ubasic_set_variable(const char* varname, const char* value, struct ubasic_ctx* ctx);
bool jump_linenum(int64_t linenum, struct ubasic_ctx* ctx);
void ubasic_set_string_variable(const char* var, const char* value, struct ubasic_ctx* ctx, bool local, bool global);
void ubasic_set_double_variable(const char* var, const double value, struct ubasic_ctx* ctx, bool local, bool global);
void ubasic_set_int_variable(const char* var, int64_t value, struct ubasic_ctx* ctx, bool local, bool global);
void ubasic_set_array_variable(const char* var, int64_t value, struct ubasic_ctx* ctx, bool local);

ub_return_type ubasic_get_numeric_variable(const char* var, struct ubasic_ctx* ctx, double* res);
int ubasic_get_numeric_int_variable(const char* var, struct ubasic_ctx* ctx);

int64_t expr(struct ubasic_ctx* ctx);
void double_expr(struct ubasic_ctx* ctx, double* res);
void line_statement(struct ubasic_ctx* ctx);
void statement(struct ubasic_ctx* ctx);
const char* str_expr(struct ubasic_ctx* ctx);
const char* str_varfactor(struct ubasic_ctx* ctx);
void ubasic_parse_fn(struct ubasic_ctx* ctx);
int64_t ubasic_getproccount(struct ubasic_ctx* ctx);
int64_t ubasic_getprocid(struct ubasic_ctx* ctx);
char* ubasic_getprocname(struct ubasic_ctx* ctx);
char* ubasic_dns(struct ubasic_ctx* ctx);
int64_t ubasic_rgb(struct ubasic_ctx* ctx);
void ubasic_eval_double_fn(const char* fn_name, struct ubasic_ctx* ctx, double* res);
const char* ubasic_test_string_variable(const char* var, struct ubasic_ctx* ctx);

// Builtin real functions
void ubasic_sin(struct ubasic_ctx* ctx, double* res);
void ubasic_cos(struct ubasic_ctx* ctx, double* res);
void ubasic_tan(struct ubasic_ctx* ctx, double* res);
void ubasic_pow(struct ubasic_ctx* ctx, double* res);

int str_relation(struct ubasic_ctx* ctx);
int relation(struct ubasic_ctx* ctx);
int64_t expr(struct ubasic_ctx* ctx);
const char* str_expr(struct ubasic_ctx* ctx);
void double_expr(struct ubasic_ctx* ctx, double* res);

void accept(int token, struct ubasic_ctx* ctx);