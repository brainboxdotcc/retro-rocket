#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/*
 * Builtin real (double) functions
 */
void basic_sin(struct basic_ctx* ctx, double* res);
void basic_cos(struct basic_ctx* ctx, double* res);
void basic_tan(struct basic_ctx* ctx, double* res);
void basic_pow(struct basic_ctx* ctx, double* res);
void basic_sqrt(struct basic_ctx* ctx, double* res);
void basic_atan(struct basic_ctx* ctx, double* res);
void basic_atan2(struct basic_ctx* ctx, double* res);
void basic_ceil(struct basic_ctx* ctx, double* res);
void basic_round(struct basic_ctx* ctx, double* res);
void basic_fmod(struct basic_ctx* ctx, double* res);
void basic_asn(struct basic_ctx* ctx, double* res);
void basic_acs(struct basic_ctx* ctx, double* res);
void basic_exp(struct basic_ctx* ctx, double* res);
void basic_log(struct basic_ctx* ctx, double* res);
void basic_deg(struct basic_ctx* ctx, double* res);
void basic_rad(struct basic_ctx* ctx, double* res);
void basic_realval(struct basic_ctx* ctx, double* res);
