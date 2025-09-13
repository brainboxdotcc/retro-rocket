/**
 * @file basic/variable.c
 * @brief BASIC variable assignment and manipulation functions
 */
#include <kernel.h>

extern bool debug;

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

void  let_statement(struct basic_ctx* ctx) {
	accept_or_return(LET, ctx);
	assignment_statement(ctx, false, false);
}

void variable_statement(struct basic_ctx* ctx) {
	assignment_statement(ctx, false, false);
}

void global_statement(struct basic_ctx* ctx) {
	accept_or_return(GLOBAL, ctx);
	assignment_statement(ctx, true, false);
}

void local_statement(struct basic_ctx* ctx) {
	accept_or_return(LOCAL, ctx);
	assignment_statement(ctx, false, true);
}

void newline_statement(struct basic_ctx* ctx) {
}

void assignment_statement(struct basic_ctx* ctx, bool global, bool local)
{
	const char* var;
	const char* _expr;
	double f_expr = 0;

	basic_debug("LET statement\n");

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
	} else if (varname_is_string_array_access(ctx, var)) {
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
	} else if (varname_is_double_array_access(ctx, var)) {
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
			basic_debug("Setting string variable '%s'\n", var);
			basic_set_string_variable(var, _expr, ctx, local, global);
		break;
		case '#':
			basic_debug("Setting double variable '%s'\n", var);
			double_expr(ctx, &f_expr);
			basic_set_double_variable(var, f_expr, ctx, local, global);
		break;
		default:
			basic_debug("Setting int variable '%s' local=%d global=%d\n", var, local, global);
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

static void update_string(struct basic_ctx* ctx, ub_var_string* str, size_t len, bool propagate_global, const char* varname, const char* value) {
	str->name_length = len;
	str->global = propagate_global;
	str->varname = buddy_strdup(ctx->allocator, varname);
	str->value = buddy_strdup(ctx->allocator, value);
}

void basic_set_string_variable(const char* var, const char* value, struct basic_ctx* ctx, bool local, bool propagate_global) {
	struct hashmap* locals = ctx->local_string_variables[ctx->call_stack_ptr];
	struct hashmap* globals = ctx->str_variables;

	size_t len = strlen(var);
	if (!valid_string_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}

	ub_var_string* found = NULL;
	bool oom = false;
	if (local && locals && (found = hashmap_get(locals, &(ub_var_string) { .varname = var }))) {
		buddy_free(ctx->allocator, found->varname);
		buddy_free(ctx->allocator, found->value);
		update_string(ctx, found, len, propagate_global, var, value);
		oom = !hashmap_set(locals, found) && hashmap_oom(locals);
	} else if ((found = hashmap_get(globals, &(ub_var_string) { .varname = var }))) {
		buddy_free(ctx->allocator, found->varname);
		buddy_free(ctx->allocator, found->value);
		update_string(ctx, found, len, propagate_global, var, value);
		oom = !hashmap_set(globals, found) && hashmap_oom(globals);
	} else {
		struct hashmap* target = local && locals ? locals : globals;
		ub_var_string new;
		update_string(ctx, &new, len, propagate_global, var, value);
		oom = !hashmap_set(target, &new) && hashmap_oom(target);
	}
	if (oom) {
		tokenizer_error_print(ctx, "Out of memory");
		return;
	}
}

static void update_int(struct basic_ctx* ctx, ub_var_int* integer, size_t len, bool propagate_global, const char* varname, int64_t value) {
	integer->name_length = len;
	integer->global = propagate_global;
	integer->varname = buddy_strdup(ctx->allocator, varname);
	integer->value = value;
}

void basic_set_int_variable(const char* var, int64_t value, struct basic_ctx* ctx, bool local, bool propagate_global) {
	struct hashmap* locals = ctx->local_int_variables[ctx->call_stack_ptr];
	struct hashmap* globals = ctx->int_variables;

	size_t len = strlen(var);
	if (!valid_int_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}

	ub_var_int* found = NULL;
	bool oom = false;
	if (local && locals && (found = hashmap_get(locals, &(ub_var_int) { .varname = var }))) {
		basic_debug("local set '%s' %p\n", var, ctx->allocator);
		buddy_free(ctx->allocator, found->varname);
		update_int(ctx, found, len, propagate_global, var, value);
		oom = !hashmap_set(locals, found) && hashmap_oom(locals);
	} else if ((found = hashmap_get(globals, &(ub_var_int) { .varname = var }))) {
		basic_debug("global set '%s' %p %p\n", var, found->varname, ctx->allocator);
		buddy_free(ctx->allocator, found->varname);
		update_int(ctx, found, len, propagate_global, var, value);
		oom = !hashmap_set(globals, found) && hashmap_oom(globals);
		basic_debug("set, oom=%d\n", oom);
	} else {
		struct hashmap* target = local && locals ? locals : globals;
		basic_debug("%s create '%s' %p\n", target == globals ? "global" : "local", var, ctx->allocator);
		ub_var_int new;
		update_int(ctx, &new, len, propagate_global, var, value);
		oom = !hashmap_set(target, &new) && hashmap_oom(target);
	}
	if (oom) {
		tokenizer_error_print(ctx, "Out of memory");
		return;
	}
}

static void update_double(struct basic_ctx* ctx, ub_var_double* dbl, size_t len, bool propagate_global, const char* varname, double value) {
	dbl->name_length = len;
	dbl->global = propagate_global;
	dbl->varname = buddy_strdup(ctx->allocator, varname);
	dbl->value = value;
}

void basic_set_double_variable(const char* var, double value, struct basic_ctx* ctx, bool local, bool propagate_global)
{
	struct hashmap* locals = ctx->local_double_variables[ctx->call_stack_ptr];
	struct hashmap* globals = ctx->double_variables;

	size_t len = strlen(var);
	if (!valid_double_var(var)) {
		tokenizer_error_printf(ctx, "Malformed variable name '%s'", var);
		return;
	}

	ub_var_double* found = NULL;
	bool oom = false;
	if (local && locals && (found = hashmap_get(locals, &(ub_var_double) { .varname = var }))) {
		buddy_free(ctx->allocator, found->varname);
		update_double(ctx, found, len, propagate_global, var, value);
		oom = !hashmap_set(locals, found) && hashmap_oom(locals);
	} else if ((found = hashmap_get(globals, &(ub_var_double) { .varname = var }))) {
		buddy_free(ctx->allocator, found->varname);
		update_double(ctx, found, len, propagate_global, var, value);
		oom = !hashmap_set(globals, found) && hashmap_oom(globals);
	} else {
		struct hashmap* target = local && locals ? locals : globals;
		ub_var_double new;
		update_double(ctx, &new, len, propagate_global, var, value);
		oom = !hashmap_set(target, &new) && hashmap_oom(target);
	}
	if (oom) {
		tokenizer_error_print(ctx, "Out of memory");
		return;
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

const char* basic_get_string_variable(const char* var, struct basic_ctx* ctx)
{
	char* retv;
	if (basic_builtin_str_fn(var, ctx, &retv)) {
		return retv;
	} else if (varname_is_string_function(var)) {
		const char* res = basic_eval_str_fn(var, ctx);
		return res;
	} else if (varname_is_string_array_access(ctx, var)) {
		return basic_get_string_array_variable(var, arr_variable_index(ctx), ctx);
	}

	ub_var_string* found = NULL;
	for (size_t j = ctx->call_stack_ptr; j > 0; --j) {
		struct hashmap* list = ctx->local_string_variables[j];
		if (list && (found = hashmap_get(list, &(ub_var_string) { .varname = var }))) {
			return found->value;
		}
	}
	if ((found = hashmap_get(ctx->str_variables, &(ub_var_string) { .varname = var }))) {
		return found->value;
	}

	tokenizer_error_printf(ctx, "No such string variable '%s'", var);
	return "";
}

bool basic_double_variable_exists(const char* var, struct basic_ctx* ctx)
{
	ub_var_double* found = NULL;
	for (size_t j = ctx->call_stack_ptr; j > 0; --j) {
		struct hashmap* list = ctx->local_double_variables[j];
		if (list && (found = hashmap_get(list, &(ub_var_double) { .varname = var }))) {
			return true;
		}
	}
	if ((found = hashmap_get(ctx->double_variables, &(ub_var_double) { .varname = var }))) {
		return true;
	}
	return false;
}

bool basic_string_variable_exists(const char* var, struct basic_ctx* ctx)
{
	ub_var_string* found = NULL;
	for (size_t j = ctx->call_stack_ptr; j > 0; --j) {
		struct hashmap* list = ctx->local_string_variables[j];
		if (list && (found = hashmap_get(list, &(ub_var_string) { .varname = var }))) {
			return true;
		}
	}
	if ((found = hashmap_get(ctx->str_variables, &(ub_var_string) { .varname = var }))) {
		return true;
	}
	return false;
}

bool basic_int_variable_exists(const char* var, struct basic_ctx* ctx)
{
	ub_var_int* found = NULL;
	for (size_t j = ctx->call_stack_ptr; j > 0; --j) {
		struct hashmap* list = ctx->local_int_variables[j];
		if (list && (found = hashmap_get(list, &(ub_var_int) { .varname = var }))) {
			return true;
		}
	}
	if ((found = hashmap_get(ctx->int_variables, &(ub_var_int) { .varname = var }))) {
		return true;
	}
	return false;
}

int64_t basic_get_int_variable(const char* var, struct basic_ctx* ctx)
{
	int64_t retv = 0;
	if (basic_builtin_int_fn(var, ctx, &retv)) {
		return retv;
	} else if (varname_is_function(var)) {
		return basic_eval_int_fn(var, ctx);
	} else if (varname_is_int_array_access(ctx, var)) {
		return basic_get_int_array_variable(var, arr_variable_index(ctx), ctx);
	}

	ub_var_int* found = NULL;
	for (size_t j = ctx->call_stack_ptr; j > 0; --j) {
		struct hashmap* list = ctx->local_int_variables[j];
		if (list && (found = hashmap_get(list, &(ub_var_int) { .varname = var }))) {
			return found->value;
		}
	}
	if ((found = hashmap_get(ctx->int_variables, &(ub_var_int) { .varname = var }))) {
		return found->value;
	}

	tokenizer_error_printf(ctx, "No such integer variable '%s'", var);
	return 0; /* No such variable */
}

bool basic_get_double_variable(const char* var, struct basic_ctx* ctx, double* res)
{
	if (basic_builtin_double_fn(var, ctx, res)) {
		return true;
	} else if (varname_is_double_function(var)) {
		basic_eval_double_fn(var, ctx, res);
		return true;
	} else if (varname_is_double_array_access(ctx, var)) {
		return basic_get_double_array_variable(var, arr_variable_index(ctx), ctx, res);
	}

	size_t len = strlen(var);
	ub_var_double* found = NULL;
	for (size_t j = ctx->call_stack_ptr; j > 0; --j) {
		struct hashmap* list = ctx->local_double_variables[j];
		if (list && (found = hashmap_get(list, &(ub_var_double) { .varname = var }))) {
			*res = found->value;
			return true;
		}
	}
	if ((found = hashmap_get(ctx->double_variables, &(ub_var_double) { .varname = var }))) {
		*res = found->value;
		return true;
	}

	if (var[len - 1] == '#') {
		tokenizer_error_printf(ctx, "No such real variable '%s'", var);
		*res = 0.0; /* No such variable */
	}
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

