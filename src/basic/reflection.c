/**
 * @file basic/reflection.c
 * @brief BASIC reflection functions
 */
#include <kernel.h>
#include <cpuid.h>

int64_t basic_getvar_int(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* var = strval;
	PARAMS_END("GETVARI", 0);
	return basic_get_int_variable(var, ctx);
}

void basic_getvar_real(struct basic_ctx* ctx, double* res)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* var = strval;
	PARAMS_END_VOID("GETVARR");
	basic_get_double_variable(var, ctx, res);
}

char* basic_getvar_string(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* var = strval;
	PARAMS_END("GETVARS", 0);
	return gc_strdup(basic_get_string_variable(var, ctx));
}

int64_t basic_existsvar_int(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* var = strval;
	PARAMS_END("EXISTSVARI", 0);
	return basic_int_variable_exists(var, ctx);
}

int64_t basic_existsvar_real(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* var = strval;
	PARAMS_END("EXISTSVARR", 0);
	return basic_double_variable_exists(var, ctx);
}

int64_t basic_existsvar_string(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_STRING);
	const char* var = strval;
	PARAMS_END("EXISTSVARS", 0);
	return basic_string_variable_exists(var, ctx);
}

void setvari_statement(struct basic_ctx* ctx)
{
	accept_or_return(SETVARI, ctx);
	const char* var = str_expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t option_global = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t option_local = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t value = expr(ctx);
	accept_or_return(NEWLINE, ctx);
	basic_set_int_variable(var, value, ctx, option_local, option_global);	
}

void setvarr_statement(struct basic_ctx* ctx)
{
	accept_or_return(SETVARR, ctx);
  /* TODO: Insert hash character at end if it missing */
	const char* var = str_expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t option_global = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t option_local = expr(ctx);
	accept_or_return(COMMA, ctx);
	double value;
	double_expr(ctx, &value);
	accept_or_return(NEWLINE, ctx);
	basic_set_double_variable(var, value, ctx, option_local, option_global);
}

void setvars_statement(struct basic_ctx* ctx)
{
	accept_or_return(SETVARS, ctx);
  /* TODO: Insert dollar character at end if it missing */
	const char* var = str_expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t option_global = expr(ctx);
	accept_or_return(COMMA, ctx);
	int64_t option_local = expr(ctx);
	accept_or_return(COMMA, ctx);
	const char* value = str_expr(ctx);
	accept_or_return(NEWLINE, ctx);
	basic_set_string_variable(var, value, ctx, option_local, option_global);	
}
