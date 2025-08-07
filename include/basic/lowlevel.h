#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/* Low level statements */
void write_cpuid(struct basic_ctx* ctx, int leaf);
void write_cpuidex(struct basic_ctx* ctx, int leaf, int subleaf);
int64_t get_cpuid_reg(struct basic_ctx* ctx, int64_t reg);
int64_t basic_legacy_cpuid(struct basic_ctx* ctx);
int64_t basic_legacy_getlastcpuid(struct basic_ctx* ctx);
char* basic_cpugetbrand(struct basic_ctx* ctx);
char* basic_cpugetvendor(struct basic_ctx* ctx);
char* basic_intoasc(struct basic_ctx* ctx);
int64_t basic_cpuid(struct basic_ctx* ctx);
