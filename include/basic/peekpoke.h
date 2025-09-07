/**
 * @file basic/peekpoke.h
 * @brief Functions related to BASIC PEEK and POKE functions
 */
#pragma once
#include <kernel.h>

/**
 * Read one byte from the given memory address.
 *
 * @param ctx BASIC context
 * @return Value read as integer, or 0 with error set if invalid
 */
int64_t basic_peek(struct basic_ctx *ctx);

/**
 * Read one 16-bit word from the given memory address.
 *
 * @param ctx BASIC context
 * @return Value read as integer, or 0 with error set if invalid
 */
int64_t basic_peekw(struct basic_ctx *ctx);

/**
 * Read one 32-bit double word from the given memory address.
 *
 * @param ctx BASIC context
 * @return Value read as integer, or 0 with error set if invalid
 */
int64_t basic_peekd(struct basic_ctx *ctx);

/**
 * Read one 64-bit quad word from the given memory address.
 *
 * @param ctx BASIC context
 * @return Value read as integer, or 0 with error set if invalid
 */
int64_t basic_peekq(struct basic_ctx *ctx);

/**
 * Store one byte at the given memory address.
 *
 * @param ctx BASIC context
 */
void poke_statement(struct basic_ctx *ctx);

/**
 * Store one 16-bit word at the given memory address.
 *
 * @param ctx BASIC context
 */
void pokew_statement(struct basic_ctx *ctx);

/**
 * Store one 32-bit double word at the given memory address.
 *
 * @param ctx BASIC context
 */
void poked_statement(struct basic_ctx *ctx);

/**
 * Store one 64-bit quad word at the given memory address.
 *
 * @param ctx BASIC context
 */
void pokeq_statement(struct basic_ctx *ctx);

void modload_statement(struct basic_ctx* ctx);

void modunload_statement(struct basic_ctx* ctx);