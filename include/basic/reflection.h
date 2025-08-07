#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/* Reflection stuff */
int64_t basic_getvar_int(struct basic_ctx* ctx);
void basic_getvar_real(struct basic_ctx* ctx, double* res);
char* basic_getvar_string(struct basic_ctx* ctx);
int64_t basic_existsvar_int(struct basic_ctx* ctx);
int64_t basic_existsvar_real(struct basic_ctx* ctx);
int64_t basic_existsvar_string(struct basic_ctx* ctx);
void setvari_statement(struct basic_ctx* ctx);
void setvarr_statement(struct basic_ctx* ctx);
void setvars_statement(struct basic_ctx* ctx);

