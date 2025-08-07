#pragma once
#include <stdint.h>
#include <stdbool.h>

struct basic_ctx;

/*
 * File I/O functions
 */
const char* make_full_path(struct basic_ctx* ctx, const char* relative);
void openin_statement(struct basic_ctx* ctx);
void openup_statement(struct basic_ctx* ctx);
void openout_statement(struct basic_ctx* ctx);
void read_statement(struct basic_ctx* ctx);
void close_statement(struct basic_ctx* ctx);
void eof_statement(struct basic_ctx* ctx);
void delete_statement(struct basic_ctx* ctx);
void mkdir_statement(struct basic_ctx* ctx);
void mount_statement(struct basic_ctx* ctx);
void rmdir_statement(struct basic_ctx* ctx);
void write_statement(struct basic_ctx* ctx);
void chdir_statement(struct basic_ctx* ctx);
char* basic_filetype(struct basic_ctx* ctx);
void data_statement(struct basic_ctx* ctx);
void restore_statement(struct basic_ctx* ctx);

