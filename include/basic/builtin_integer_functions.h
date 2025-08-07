#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/*
 * Builtin integer functions
 */
int64_t basic_abs(struct basic_ctx* ctx);
int64_t basic_len(struct basic_ctx* ctx);
int64_t basic_openin(struct basic_ctx* ctx);
int64_t basic_openout(struct basic_ctx* ctx);
int64_t basic_openup(struct basic_ctx* ctx);
int64_t basic_eof(struct basic_ctx* ctx);
int64_t basic_read(struct basic_ctx* ctx);
int64_t basic_instr(struct basic_ctx* ctx);
int64_t basic_asc(struct basic_ctx* ctx);
int64_t basic_getnamecount(struct basic_ctx* ctx);
int64_t basic_getsize(struct basic_ctx* ctx);
int64_t basic_get_text_max_x(struct basic_ctx* ctx);
int64_t basic_get_text_max_y(struct basic_ctx* ctx);
int64_t basic_get_text_cur_x(struct basic_ctx* ctx);
int64_t basic_get_text_cur_y(struct basic_ctx* ctx);
int64_t basic_getproccount(struct basic_ctx* ctx);
int64_t basic_getprocid(struct basic_ctx* ctx);
int64_t basic_getprocparent(struct basic_ctx* ctx);
int64_t basic_getproccpuid(struct basic_ctx* ctx);
int64_t basic_getprocmem(struct basic_ctx* ctx);
int64_t basic_rgb(struct basic_ctx* ctx);
int64_t basic_get_free_mem(struct basic_ctx* ctx);
int64_t basic_get_used_mem(struct basic_ctx* ctx);
int64_t basic_get_total_mem(struct basic_ctx* ctx);
int64_t basic_get_program_peak_mem(struct basic_ctx* ctx);
int64_t basic_get_program_cur_mem(struct basic_ctx* ctx);
int64_t basic_sockstatus(struct basic_ctx* ctx);
int64_t basic_ctrlkey(struct basic_ctx* ctx);
int64_t basic_shiftkey(struct basic_ctx* ctx);
int64_t basic_altkey(struct basic_ctx* ctx);
int64_t basic_capslock(struct basic_ctx* ctx);
int64_t basic_random(struct basic_ctx* ctx);
int64_t basic_legacy_getlastcpuid(struct basic_ctx* ctx);
int64_t basic_legacy_cpuid(struct basic_ctx* ctx);
int64_t basic_cpuid(struct basic_ctx* ctx);
int64_t basic_atoi(struct basic_ctx* ctx);
int64_t basic_shl(struct basic_ctx* ctx);
int64_t basic_shr(struct basic_ctx* ctx);
int64_t basic_sgn(struct basic_ctx* ctx);
int64_t basic_int(struct basic_ctx* ctx);
int64_t basic_val(struct basic_ctx* ctx);
int64_t basic_hexval(struct basic_ctx* ctx);
int64_t basic_octval(struct basic_ctx* ctx);
