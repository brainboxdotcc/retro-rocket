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
#ifndef __UBASIC_H__
#define __UBASIC_H__

#define MAX_STRINGLEN 1024
#define MAX_GOSUB_STACK_DEPTH 255
#define MAX_FOR_STACK_DEPTH 50

struct for_state
{
        s64 line_after_for;
        char* for_variable;
        s64 to;
        s64 step;
};

struct ub_var_int
{
	char* varname;
	s64 value;
	struct ub_var_int* next;
};

struct ub_var_string
{
	char* varname; /* Not including the $ on the end! */
	char* value;
	struct ub_var_string* next;
};

typedef enum
{
	FT_FN,
	FT_PROC
} ub_fn_type;

typedef enum
{
	RT_STRING,
	RT_INT
} ub_return_type;

struct ub_param
{
	char* name;
	struct ub_param* next;
};

struct ub_proc_fn_def
{
	char* name;
	ub_fn_type type;
	s64 line;
	struct ub_param* params;
	struct ub_proc_fn_def* next;
};

struct ub_var_int_array
{
	char* varname;
	struct ub_var_int* values;
	u64 itemcount;
};

struct ub_var_string_array
{
	char* varname;
	struct ub_var_string* values;
	u64 itemcount;
};

struct ubasic_ctx
{
        char const *ptr;
	char const* nextptr;
        int current_token;
	s64 current_linenum;
	int errored;
        char *program_ptr;
        char string[MAX_STRINGLEN];
        int gosub_stack[MAX_GOSUB_STACK_DEPTH];
	struct ub_var_int* local_int_variables[MAX_GOSUB_STACK_DEPTH];
	struct ub_var_string* local_string_variables[MAX_GOSUB_STACK_DEPTH];
        u64 gosub_stack_ptr;
	int oldlen;
	s64 eval_linenum;
        struct for_state for_stack[MAX_FOR_STACK_DEPTH];
        u64 for_stack_ptr;
	struct ub_proc_fn_def* defs;
        struct ub_var_int* int_variables;
	struct ub_var_string* str_variables;
	struct ub_var_int_array* int_array_variables;
	struct ub_var_string_array* string_array_variables;
	struct console* cons;
        int ended;
	ub_return_type fn_type;
	void* fn_return;
	int bracket_depth;
	char const* item_begin;
	struct ub_param* param;
};

// Builtin integer functions
s64 ubasic_abs(struct ubasic_ctx* ctx);
s64 ubasic_len(struct ubasic_ctx* ctx);
s64 ubasic_openin(struct ubasic_ctx* ctx);
s64 ubasic_eof(struct ubasic_ctx* ctx);
s64 ubasic_read(struct ubasic_ctx* ctx);
s64 ubasic_instr(struct ubasic_ctx* ctx);
s64 ubasic_asc(struct ubasic_ctx* ctx);
s64 ubasic_getnamecount(struct ubasic_ctx* ctx);
s64 ubasic_getsize(struct ubasic_ctx* ctx);

// Builtin string functions
char* ubasic_left(struct ubasic_ctx* ctx);
char* ubasic_chr(struct ubasic_ctx* ctx);
char* ubasic_readstring(struct ubasic_ctx* ctx);
char* ubasic_getname(struct ubasic_ctx* ctx);

typedef s64 (*builtin_int_fn)(struct ubasic_ctx* ctx);
typedef char* (*builtin_str_fn)(struct ubasic_ctx* ctx);

struct ubasic_int_fn
{
	builtin_int_fn handler;
	const char* name;
};

struct ubasic_str_fn
{
	builtin_str_fn handler;
	const char* name;
};

struct ubasic_ctx* ubasic_init(const char *program, console* cons);
void ubasic_destroy(struct ubasic_ctx* ctx);
void ubasic_run(struct ubasic_ctx* ctx);
int ubasic_finished(struct ubasic_ctx* ctx);
s64 ubasic_get_int_variable(const char* varname, struct ubasic_ctx* ctx);
const char* ubasic_get_string_variable(const char* var, struct ubasic_ctx* ctx);
void ubasic_set_variable(const char* varname, const char* value, struct ubasic_ctx* ctx);
void jump_linenum(s64 linenum, struct ubasic_ctx* ctx);
void ubasic_set_string_variable(const char* var, const char* value, struct ubasic_ctx* ctx, int local);
void ubasic_set_int_variable(const char* var, int value, struct ubasic_ctx* ctx, int local);
void ubasic_set_array_variable(const char* var, int value, struct ubasic_ctx* ctx, int local);

#endif /* __UBASIC_H__ */
