#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/*
 * Builtin string functions
 */
char* basic_netinfo(struct basic_ctx* ctx);
char* basic_left(struct basic_ctx* ctx);
char* basic_right(struct basic_ctx* ctx);
char* basic_mid(struct basic_ctx* ctx);
char* basic_chr(struct basic_ctx* ctx);
char* basic_readstring(struct basic_ctx* ctx);
char* basic_getname(struct basic_ctx* ctx);
char* basic_getprocname(struct basic_ctx* ctx);
char* basic_getprocstate(struct basic_ctx* ctx);
char* basic_dns(struct basic_ctx* ctx);
char* basic_ramdisk_from_device(struct basic_ctx* ctx);
char* basic_ramdisk_from_size(struct basic_ctx* ctx);
char* basic_inkey(struct basic_ctx* ctx);
char* basic_insocket(struct basic_ctx* ctx);
char* basic_upper(struct basic_ctx* ctx);
char* basic_lower(struct basic_ctx* ctx);
char* basic_tokenize(struct basic_ctx* ctx);
char* basic_csd(struct basic_ctx* ctx);
char* basic_cpugetbrand(struct basic_ctx* ctx);
char* basic_cpugetvendor(struct basic_ctx* ctx);
char* basic_intoasc(struct basic_ctx* ctx);
char* basic_ljust(struct basic_ctx* ctx);
char* basic_rjust(struct basic_ctx* ctx);
char* basic_ltrim(struct basic_ctx* ctx);
char* basic_rtrim(struct basic_ctx* ctx);
char* basic_trim(struct basic_ctx* ctx);
char* basic_itoa(struct basic_ctx* ctx);
char* basic_repeat(struct basic_ctx* ctx);
char* basic_reverse(struct basic_ctx* ctx);
char* basic_str(struct basic_ctx* ctx);
char* basic_bool(struct basic_ctx* ctx);
