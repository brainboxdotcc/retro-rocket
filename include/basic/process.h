#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/* Process/memory functions */
int64_t basic_getproccount(struct basic_ctx* ctx);
int64_t basic_get_free_mem(struct basic_ctx* ctx);
int64_t basic_get_used_mem(struct basic_ctx* ctx);
int64_t basic_get_total_mem(struct basic_ctx* ctx);
int64_t basic_getprocid(struct basic_ctx* ctx);
char* basic_getprocname(struct basic_ctx* ctx);
int64_t basic_getprocparent(struct basic_ctx* ctx);
int64_t basic_getproccpuid(struct basic_ctx* ctx);
