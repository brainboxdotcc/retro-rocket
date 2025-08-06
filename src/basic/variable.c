/**
 * @file basic/variable.c
 * @brief BASIC variable assignment and manipulation functions
 */
#include <kernel.h>

const struct g_cpuid_vendor cpuid_vendors[] =
{
	{ "VENDOR_AMD_K5$",     "AMDisbetter!" },
	{ "VENDOR_AMD$",        "AuthenticAMD" },
	{ "VENDOR_CENTAUR$",    "CentaurHauls" },
	{ "VENDOR_CYRIX$",      "CyrixInstead" },
	{ "VENDOR_INTEL$",      "GenuineIntel" },
	{ "VENDOR_TRANSMETA$",  "GenuineTMx86" },
	{ "VENDOR_TRANSMETAS$", "TransmetaCPU" },
	{ "VENDOR_NSC$",        "Geode by NSC" },
	{ "VENDOR_NEXGEN$",     "NexGenDriven" },
	{ "VENDOR_RISE$",       "RiseRiseRise" },
	{ "VENDOR_SIS$",        "SiS SiS SiS " },
	{ "VENDOR_UMC$",        "UMC UMC UMC " },
	{ "VENDOR_VIA$",        "VIA VIA VIA " },
	{ "VENDOR_VORTEX86$",   "Vortex86 SoC" },
	{ "VENDOR_ZHAOXIN$",    "  Shanghai  " },
	{ "VENDOR_HYGON$",      "HygonGenuine" },
	{ "VENDOR_RDC$",        "Genuine  RDC" },
	{ "VENDOR_BHYVE$",      "bhyve bhyve " },
	{ "VENDOR_KVM$",        " KVMKVMKVM  " },
	{ "VENDOR_QEMU$",       "TCGTCGTCGTCG" },
	{ "VENDOR_HYPERV$",     "Microsoft Hv" },
	{ "VENDOR_XTA$",        "MicrosoftXTA" },
	{ "VENDOR_PARALLELS$",  " lrpepyh  vr" },
	{ "VENDOR_FPARALLELS$", "prl hyperv  " },
	{ "VENDOR_VMWARE$",     "VMwareVMware" },
	{ "VENDOR_XEN$",        "XenVMMXenVMM" },
	{ "VENDOR_ACRN$",       "ACRNACRNACRN" },
	{ "VENDOR_QNX$",        " QNXQVMBSQG " },
	{ NULL,                NULL }
};

void set_system_variables(struct basic_ctx* ctx, uint32_t pid)
{
	const struct g_cpuid_vendor* p = &cpuid_vendors[0];
	while (p->varname != NULL &&
		   p->vendor != NULL) {
		basic_set_string_variable(p->varname, p->vendor, ctx, false, false);
		++p;
	}
	basic_set_int_variable("TRUE", 1, ctx, false, false);
	basic_set_int_variable("FALSE", 0, ctx, false, false);
	basic_set_int_variable("PID", pid, ctx, false, false);
	basic_set_int_variable("GRAPHICS_WIDTH", screen_get_width(), ctx, false, false);
	basic_set_int_variable("GRAPHICS_HEIGHT", screen_get_height(), ctx, false, false);
	basic_set_int_variable("GRAPHICS_CENTRE_X", screen_get_width() / 2, ctx, false, false);
	basic_set_int_variable("GRAPHICS_CENTRE_Y", screen_get_height() / 2, ctx, false, false);
	basic_set_int_variable("GRAPHICS_CENTER_X", screen_get_width() / 2, ctx, false, false);
	basic_set_int_variable("GRAPHICS_CENTER_Y", screen_get_height() / 2, ctx, false, false);
	basic_set_double_variable("PI#", 3.141592653589793238, ctx, false, false);
	basic_set_double_variable("E#", 2.7182818284590451, ctx, false, false);
}

void let_statement(struct basic_ctx* ctx, bool global, bool local)
{
	const char* var;
	const char* _expr;
	double f_expr = 0;

	var = tokenizer_variable_name(ctx);

	if (varname_is_int_array_access(ctx, var)) {
		int64_t index = arr_expr_set_index(ctx, var);
		int64_t value = expr(ctx);
		if (index == -1) {
			basic_set_int_array(var, value, ctx);
			accept_or_return(NEWLINE, ctx);
			return;
		}
		basic_set_int_array_variable(var, index, value, ctx);
		accept_or_return(NEWLINE, ctx);
		return;
	}
	if (varname_is_string_array_access(ctx, var)) {
		int64_t index = arr_expr_set_index(ctx, var);
		const char* value = str_expr(ctx);
		if (index == -1) {
			basic_set_string_array(var, value, ctx);
			accept_or_return(NEWLINE, ctx);
			return;
		}
		basic_set_string_array_variable(var, index, value, ctx);
		accept_or_return(NEWLINE, ctx);
		return;
	}
	if (varname_is_double_array_access(ctx, var)) {
		int64_t index = arr_expr_set_index(ctx, var);
		double value = 0;
		double_expr(ctx, &value);
		if (index == -1) {
			basic_set_double_array(var, value, ctx);
			accept_or_return(NEWLINE, ctx);
			return;
		}
		basic_set_double_array_variable(var, index, value, ctx);
		accept_or_return(NEWLINE, ctx);
		return;
	}

	accept_or_return(VARIABLE, ctx);
	accept_or_return(EQUALS, ctx);

	switch (var[strlen(var) - 1]) {
		case '$':
			_expr = str_expr(ctx);
			basic_set_string_variable(var, _expr, ctx, local, global);
		break;
		case '#':
			double_expr(ctx, &f_expr);
			basic_set_double_variable(var, f_expr, ctx, local, global);
		break;
		default:
			basic_set_int_variable(var, expr(ctx), ctx, local, global);
		break;
	}
	accept_or_return(NEWLINE, ctx);
}

bool valid_suffix_var(const char* name, char suffix)
{
	const char* i;
	unsigned int varLength = strlen(name);
	if (suffix != '\0') {
		if (varLength < 2 || name[varLength - 1] != suffix) {
			return false;
		}
		size_t offset = 0;
		for (i = name; *i != suffix; ++i) {
			if (*i == suffix && *(i + 1) != 0) {
			       return false;
			}
			if (offset > 0 && isdigit(*i)) {
				continue;
			}
			if ((*i < 'A' || *i > 'Z') && (*i < 'a' || *i > 'z') && *i != '_') {
				return false;
			}
			if (offset > 60) {
				return false;
			}
			++offset;
		}
		return true;
	}
	if (varLength < 1) {
		return false;
	}
	size_t offset = 0;
	for (i = name; *i; ++i) {
		if (offset > 0 && isdigit(*i)) {
			continue;
		}
		if ((*i < 'A' || *i > 'Z') && (*i < 'a' || *i > 'z') && *i != '_') {
			return false;
		}
		if (offset > 60) {
			return false;
		}
		++offset;
	}
	return true;
}

bool valid_string_var(const char* name)
{
    return valid_suffix_var(name, '$');
}

bool valid_double_var(const char* name)
{
    return valid_suffix_var(name, '#');
}

bool valid_int_var(const char* name)
{
    return valid_suffix_var(name, '\0');
}

void basic_set_string_variable(const char* var, const char* value, struct basic_ctx* ctx, bool local, bool global)
{
	bool error_set = false;
	struct ub_var_string* list[] = {
		ctx->str_variables,
		ctx->local_string_variables[ctx->call_stack_ptr]
	};

	if (*value && !strcmp(var, "ERROR$")) {
		error_set = true;
	}

	if (!valid_string_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}
	size_t len = strlen(var);
	if (list[local] == NULL) {
		if (local) {
			ctx->local_string_variables[ctx->call_stack_ptr] = buddy_malloc(ctx->allocator, sizeof(struct ub_var_string));
			ctx->local_string_variables[ctx->call_stack_ptr]->next = NULL;
			ctx->local_string_variables[ctx->call_stack_ptr]->varname = buddy_strdup(ctx->allocator, var);
			ctx->local_string_variables[ctx->call_stack_ptr]->value = buddy_strdup(ctx->allocator, value);
			ctx->local_string_variables[ctx->call_stack_ptr]->global = global;
			ctx->local_string_variables[ctx->call_stack_ptr]->name_length = len;
		} else {
			ctx->str_variables = buddy_malloc(ctx->allocator, sizeof(struct ub_var_string));
			ctx->str_variables->next = NULL;
			ctx->str_variables->varname = buddy_strdup(ctx->allocator, var);
			ctx->str_variables->value = buddy_strdup(ctx->allocator, value);
			ctx->str_variables->name_length = len;
			ctx->str_variables->global = global;
		}
		return;
	} else {
		struct ub_var_string* cur = ctx->str_variables;
		if (local) {
			cur = ctx->local_string_variables[ctx->call_stack_ptr];
		}
		for (; cur; cur = cur->next) {
			if (len == cur->name_length && !strcmp(var, cur->varname))	{
				if (error_set && *cur->value) {
					/* If ERROR$ is set, can't change it except to empty */
					return;
				} else if (error_set) {
					dprintf("Set ERROR$ to: '%s'\n", value);
				}
				buddy_free(ctx->allocator, cur->value);
				cur->value = buddy_strdup(ctx->allocator, value);
				cur->global = global;
				return;
			}
		}
		struct ub_var_string* newvar = buddy_malloc(ctx->allocator, sizeof(struct ub_var_string));
		newvar->next = (local ? ctx->local_string_variables[ctx->call_stack_ptr] : ctx->str_variables);
		newvar->varname = buddy_strdup(ctx->allocator, var);
		newvar->value = buddy_strdup(ctx->allocator, value);
		newvar->name_length = len;
		newvar->global = global;
		if (local) {
			ctx->local_string_variables[ctx->call_stack_ptr] = newvar;
		} else {
			ctx->str_variables = newvar;
		}
	}
}

void basic_set_int_variable(const char* var, int64_t value, struct basic_ctx* ctx, bool local, bool global)
{
	struct ub_var_int* list[] = {
		ctx->int_variables,
		ctx->local_int_variables[ctx->call_stack_ptr]
	};

	if (!valid_int_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}
	size_t len = strlen(var);
	if (list[local] == NULL) {
		if (local) {
			ctx->local_int_variables[ctx->call_stack_ptr] = buddy_malloc(ctx->allocator, sizeof(struct ub_var_int));
			ctx->local_int_variables[ctx->call_stack_ptr]->next = NULL;
			ctx->local_int_variables[ctx->call_stack_ptr]->varname = buddy_strdup(ctx->allocator, var);
			ctx->local_int_variables[ctx->call_stack_ptr]->name_length = len;
			ctx->local_int_variables[ctx->call_stack_ptr]->value = value;
		} else {
			//dprintf("Set int variable '%s' to '%d' (default)\n", var, value);
			ctx->int_variables = buddy_malloc(ctx->allocator, sizeof(struct ub_var_int));
			ctx->int_variables->next = NULL;
			ctx->int_variables->varname = buddy_strdup(ctx->allocator, var);
			ctx->int_variables->name_length = len;
			ctx->int_variables->value = value;
		}
		return;
	} else {
		struct ub_var_int* cur = local ? ctx->local_int_variables[ctx->call_stack_ptr] : ctx->int_variables;
		for (; cur; cur = cur->next) {
			if (len == cur->name_length && !strcmp(var, cur->varname)) {
				//dprintf("Set int variable '%s' to '%d' (updating)\n", var, value);
				cur->value = value;
				return;
			}
		}
		//dprintf("Set int variable '%s' to '%d'\n", var, value);
		struct ub_var_int* newvar = buddy_malloc(ctx->allocator, sizeof(struct ub_var_int));
		newvar->next = (local ? ctx->local_int_variables[ctx->call_stack_ptr] : ctx->int_variables);
		newvar->varname = buddy_strdup(ctx->allocator, var);
		newvar->name_length = len;
		newvar->value = value;
		if (local) {
			ctx->local_int_variables[ctx->call_stack_ptr] = newvar;
		} else {
			ctx->int_variables = newvar;
		}
	}
}

void basic_set_double_variable(const char* var, double value, struct basic_ctx* ctx, bool local, bool global)
{
	struct ub_var_double* list[] = {
		ctx->double_variables,
		ctx->local_double_variables[ctx->call_stack_ptr]
	};

	if (!valid_double_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}

	size_t len = strlen(var);
	if (list[local] == NULL) {
		if (local) {
			ctx->local_double_variables[ctx->call_stack_ptr] = buddy_malloc(ctx->allocator, sizeof(struct ub_var_double));
			ctx->local_double_variables[ctx->call_stack_ptr]->next = NULL;
			ctx->local_double_variables[ctx->call_stack_ptr]->varname = buddy_strdup(ctx->allocator, var);
			ctx->local_double_variables[ctx->call_stack_ptr]->name_length = len;
			ctx->local_double_variables[ctx->call_stack_ptr]->value = value;
		} else {
			ctx->double_variables = buddy_malloc(ctx->allocator, sizeof(struct ub_var_double));
			ctx->double_variables->next = NULL;
			ctx->double_variables->varname = buddy_strdup(ctx->allocator, var);
			ctx->double_variables->name_length = len;
			ctx->double_variables->value = value;
		}
		return;
	} else {
		struct ub_var_double* cur = ctx->double_variables;
		if (local)
			cur = ctx->local_double_variables[ctx->call_stack_ptr];
		for (; cur; cur = cur->next) {
			if (len == cur->name_length && !strcmp(var, cur->varname)) {
				cur->value = value;
				return;
			}
		}
		//dprintf("Set double variable '%s' to '%s'\n", var, double_to_string(value, buffer, MAX_STRINGLEN, 0));
		struct ub_var_double* newvar = buddy_malloc(ctx->allocator, sizeof(struct ub_var_double));
		newvar->next = (local ? ctx->local_double_variables[ctx->call_stack_ptr] : ctx->double_variables);
		newvar->varname = buddy_strdup(ctx->allocator, var);
		newvar->name_length = len;
		newvar->value = value;
		if (local) {
			ctx->local_double_variables[ctx->call_stack_ptr] = newvar;
		} else {
			ctx->double_variables = newvar;
		}
	}
}

/**
 * @brief Returns true if 'varname' starts with FN
 * (is a function call)
 * 
 * @param varname variable name to check
 * @return char 1 if variable name is a function call, 0 if it is not
 */
char varname_is_function(const char* varname) {
	return (*varname == 'F' && *(varname + 1) == 'N' && !strchr(varname, '#') && !strchr(varname, '$'));
}

char varname_is_string_function(const char* varname) {
	return (*varname == 'F' && *(varname + 1) == 'N' && strchr(varname, '$') && !strchr(varname, '#'));
}

char varname_is_double_function(const char* varname) {
	return (*varname == 'F' && *(varname + 1) == 'N' && strchr(varname, '#') && !strchr(varname, '$'));
}

const char* basic_test_string_variable(const char* var, struct basic_ctx* ctx)
{
	struct ub_var_string* list[] = {
		ctx->local_string_variables[ctx->call_stack_ptr],
		ctx->str_variables
	};
	size_t len = strlen(var);
	for (int j = 0; j < 2; j++) {
		struct ub_var_string* cur = list[j];
		for (; cur; cur = cur->next) {
			if (len == cur->name_length && !strcmp(var, cur->varname)) {
				return cur->value;
			}
		}
	}
	return NULL;
}

const char* basic_get_string_variable(const char* var, struct basic_ctx* ctx)
{
	char* retv;
	if (basic_builtin_str_fn(var, ctx, &retv)) {
		return retv;
	}

	if (varname_is_string_function(var)) {
		const char* res = basic_eval_str_fn(var, ctx);
		return res;
	}

	if (varname_is_string_array_access(ctx, var)) {
		return basic_get_string_array_variable(var, arr_variable_index(ctx), ctx);
	}

	struct ub_var_string* list[ctx->call_stack_ptr + 1];
	int64_t j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_string_variables[j];
	}
	list[0] = ctx->str_variables;
	size_t len = strlen(var);

	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_string* cur = list[j];
		struct ub_var_string* prev = NULL;
		while (cur) {
			if (len == cur->name_length && !strcmp(var, cur->varname)) {
				/* move-to-front optimisation */
				if (prev) {
					/* unlink */
					prev->next = cur->next;
					/* relink at head */
					if (j == 0) {
						cur->next = ctx->str_variables;
						ctx->str_variables = cur;
					} else {
						cur->next = ctx->local_string_variables[j];
						ctx->local_string_variables[j] = cur;
					}
				}
				return cur->value;
			}
			prev = cur;
			cur = cur->next;
		}
	}

	tokenizer_error_printf(ctx, "No such string variable '%s'", var);
	return "";
}

bool basic_double_variable_exists(const char* var, struct basic_ctx* ctx)
{
	struct ub_var_double* list[ctx->call_stack_ptr + 1];
	int64_t j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_double_variables[j];
	}
	list[0] = ctx->double_variables;
	size_t len = strlen(var);
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_double* cur = list[j];
		for (; cur; cur = cur->next) {
			if (len == cur->name_length && !strcmp(var, cur->varname))	{
				return true;
			}
		}
	}
	return false;
}

bool basic_string_variable_exists(const char* var, struct basic_ctx* ctx)
{
	struct ub_var_string* list[ctx->call_stack_ptr + 1];
	int64_t j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_string_variables[j];
	}
	list[0] = ctx->str_variables;
	size_t len = strlen(var);
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_string* cur = list[j];
		for (; cur; cur = cur->next) {
			if (len == cur->name_length && !strcmp(var, cur->varname)) {
				return true;
			}
		}
	}
	return false;
}

bool basic_int_variable_exists(const char* var, struct basic_ctx* ctx)
{
	struct ub_var_int* list[ctx->call_stack_ptr + 1];
	int64_t j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_int_variables[j];
	}
	list[0] = ctx->int_variables;
	size_t len = strlen(var);
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_int* cur = list[j];
		for (; cur; cur = cur->next) {
			if (len == cur->name_length && !strcmp(var, cur->varname)) {
				return true;
			}
		}
	}
	return false;
}

int64_t basic_get_int_variable(const char* var, struct basic_ctx* ctx)
{
	int64_t retv = 0;
	if (basic_builtin_int_fn(var, ctx, &retv)) {
		return retv;
	}

	if (varname_is_function(var)) {
		return basic_eval_int_fn(var, ctx);
	}

	if (varname_is_int_array_access(ctx, var)) {
		return basic_get_int_array_variable(var, arr_variable_index(ctx), ctx);
	}

	struct ub_var_int* list[ctx->call_stack_ptr + 1];
	int64_t j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_int_variables[j];
	}
	list[0] = ctx->int_variables;

	size_t len = strlen(var);
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_int* cur = list[j];
		struct ub_var_int* prev = NULL;
		while (cur) {
			if (len == cur->name_length && !strcmp(var, cur->varname)) {
				/* If ERROR is read, it resets its value */
				int64_t v = cur->value;
				if (len == 5 && !strcmp(var, "ERROR")) {
					basic_set_int_variable("ERROR", 0, ctx, false, false);
				}

				/* move-to-front optimisation */
				if (prev) {
					/* unlink */
					prev->next = cur->next;
					/* relink at head */
					if (j == 0) {
						cur->next = ctx->int_variables;
						ctx->int_variables = cur;
					} else {
						cur->next = ctx->local_int_variables[j];
						ctx->local_int_variables[j] = cur;
					}
				}
				return v;
			}
			prev = cur;
			cur = cur->next;
		}
	}

	tokenizer_error_printf(ctx, "No such integer variable '%s'", var);
	return 0; /* No such variable */
}

bool basic_get_double_variable(const char* var, struct basic_ctx* ctx, double* res)
{
	if (basic_builtin_double_fn(var, ctx, res)) {
		return true;
	}

	if (varname_is_double_function(var)) {
		basic_eval_double_fn(var, ctx, res);
		return true;
	}

	if (varname_is_double_array_access(ctx, var)) {
		return basic_get_double_array_variable(var, arr_variable_index(ctx), ctx, res);
	}

	struct ub_var_double* list[ctx->call_stack_ptr + 1];
	int64_t j;
	for (j = ctx->call_stack_ptr; j > 0; --j) {
		list[j] = ctx->local_double_variables[j];
	}
	list[0] = ctx->double_variables;

	size_t len = strlen(var);
	for (j = ctx->call_stack_ptr; j >= 0; --j) {
		struct ub_var_double* cur = list[j];
		struct ub_var_double* prev = NULL;
		while (cur) {
			if (len == cur->name_length && !strcmp(var, cur->varname)) {
				*res = cur->value;
				/* move-to-front optimisation */
				if (prev) {
					/* unlink */
					prev->next = cur->next;
					/* relink at head */
					if (j == 0) {
						cur->next = ctx->double_variables;
						ctx->double_variables = cur;
					} else {
						cur->next = ctx->local_double_variables[j];
						ctx->local_double_variables[j] = cur;
					}
				}
				return true;
			}
			prev = cur;
			cur = cur->next;
		}
	}

	if (var[len - 1] == '#') {
		tokenizer_error_printf(ctx, "No such real variable '%s'", var);
	}
	*res = 0.0; /* No such variable */
	return false;
}

ub_return_type basic_get_numeric_variable(const char* var, struct basic_ctx* ctx, double* res)
{
	if (basic_get_double_variable(var, ctx, res)) {
		return RT_INT;
	}
	*res = (double)(basic_get_int_variable(var, ctx));
	return RT_FLOAT;
}

int64_t basic_get_numeric_int_variable(const char* var, struct basic_ctx* ctx)
{
	double res;
	if (basic_get_double_variable(var, ctx, &res)) {
		return (int64_t)res;
	}
	return basic_get_int_variable(var, ctx);
}

