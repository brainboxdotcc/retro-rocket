#pragma once
#include <kernel.h>
#include <audio.h>

int64_t assign_stream(struct basic_ctx* ctx);

bool release_stream(struct basic_ctx* ctx, int64_t stream_id);

mixer_stream_t* get_stream(struct basic_ctx* ctx, int64_t stream_id);

void stream_statement(struct basic_ctx* ctx);

void sound_statement(struct basic_ctx* ctx);

int64_t basic_decibels(struct basic_ctx* ctx);

void sound_list_free_all(struct basic_ctx *ctx);

void stream_list_free_all(struct basic_ctx *ctx);