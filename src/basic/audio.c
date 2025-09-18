#include <kernel.h>

static inline void sound_list_add(struct basic_ctx *ctx, basic_sound_t *s) {
	s->next = ctx->sounds;
	ctx->sounds = s;
}

static inline bool sound_list_remove(struct basic_ctx *ctx, basic_sound_t *target) {
	basic_sound_t **pp = &ctx->sounds;
	while (*pp) {
		if (*pp == target) {
			*pp = target->next;
			kfree_null(&target->pcm);
			buddy_free(ctx->allocator, target);
			return true;
		}
		pp = &(*pp)->next;
	}
	return false;
}

void sound_list_free_all(struct basic_ctx *ctx) {
	basic_sound_t *cur = ctx->sounds;
	while (cur) {
		basic_sound_t *next = cur->next;

		/* free the PCM buffer first */
		kfree_null(&cur->pcm);

		/* free the node itself */
		buddy_free(ctx->allocator, cur);

		cur = next;
	}

	ctx->sounds = NULL;
}


static inline bool sound_list_validate(struct basic_ctx *ctx, basic_sound_t *target) {
	basic_sound_t **pp = &ctx->sounds;
	while (*pp) {
		if (*pp == target) {
			return true;
		}
		pp = &(*pp)->next;
	}
	return false;
}

static basic_sound_t *load_sound_from_path(struct basic_ctx* ctx, const char *path) {
	size_t size;
	void* bits;
	if (!audio_wav_load(path, &bits, &size)) {
		tokenizer_error_printf(ctx, "Unable to load WAV file '%s'", path);
		return NULL;
	}
	basic_sound_t* sound = buddy_malloc(ctx->allocator, sizeof(basic_sound_t));
	if (!sound) {
		tokenizer_error_printf(ctx, "Out of memory loading WAV file '%s'", path);
		kfree_null(&bits);
		return NULL;
	}
	sound->pcm = bits;
	sound->frames = wav_size_to_samples(size);
	sound->next = NULL;
	return sound;
}

void stream_list_free_all(struct basic_ctx* ctx) {
	for (size_t ch = 0; ch < sizeof(ctx->audio_streams) / sizeof(ctx->audio_streams[0]); ++ch) {
		if (ctx->audio_streams[ch]) {
			mixer_free_stream(ctx->audio_streams[ch]);
		}
	}
}

int64_t assign_stream(struct basic_ctx* ctx) {
	for (int64_t s = 0; s < (int64_t)sizeof(ctx->audio_streams) / (int64_t)sizeof(ctx->audio_streams[0]); ++s) {
		if (ctx->audio_streams[s] == NULL) {
			ctx->audio_streams[s] = mixer_create_stream();
			return ctx->audio_streams[s] ? s : -1;
		}
	}
	return -1;
}

bool release_stream(struct basic_ctx* ctx, int64_t stream_id) {
	const int64_t stream_max = sizeof(ctx->audio_streams) / sizeof(ctx->audio_streams[0]);
	if (stream_id < 0 || stream_id >= stream_max || ctx->audio_streams[stream_id] == NULL) {
		return false;
	}
	mixer_free_stream(ctx->audio_streams[stream_id]);
	ctx->audio_streams[stream_id] = NULL;
	return true;
}

mixer_stream_t* get_stream(struct basic_ctx* ctx, int64_t stream_id) {
	const int64_t stream_max = sizeof(ctx->audio_streams) / sizeof(ctx->audio_streams[0]);
	if (stream_id < 0 || stream_id >= stream_max) {
		return NULL;
	}
	return ctx->audio_streams[stream_id];
}

void stream_statement(struct basic_ctx* ctx) {
	if (!find_first_audio_device()) {
		tokenizer_error_print(ctx, "STREAM: No sound driver is loaded");
		return;
	}
	accept_or_return(STREAM, ctx);
	switch (tokenizer_token(ctx)) {
		case CREATE: {
			/* Create new stream */
			accept_or_return(CREATE, ctx);
			size_t var_length;
			const char* variable = tokenizer_variable_name(ctx, &var_length);
			accept_or_return(VARIABLE, ctx);
			int64_t stream_id = assign_stream(ctx);
			if (stream_id < 0) {
				tokenizer_error_print(ctx, "Out of STREAMs");
			}
			basic_set_int_variable(variable, stream_id, ctx, false, false);
			break;
		}
		case DESTROY: {
			/* Delete stream */
			accept_or_return(DESTROY, ctx);
			int64_t stream_id = expr(ctx);
			if (!release_stream(ctx, stream_id)) {
				tokenizer_error_print(ctx, "Invalid STREAM handle");
			}
			break;
		}
		default:
			tokenizer_error_print(ctx, "Expected CREATE or DESTROY after STREAM");
			return;
	}
	accept_or_return(NEWLINE, ctx);
}

void sound_statement(struct basic_ctx* ctx) {
	if (!find_first_audio_device()) {
		tokenizer_error_print(ctx, "SOUND: No sound driver is loaded");
		return;
	}
	accept_or_return(SOUND, ctx);
	switch (tokenizer_token(ctx)) {
		case VOLUME: {
			/* Set volume for stream */
			accept_or_return(VOLUME, ctx);
			mixer_stream_t* stream = get_stream(ctx, expr(ctx));
			if (!stream) {
				tokenizer_error_print(ctx, "Invalid STREAM handle");
				return;
			}
			int64_t gain = expr(ctx);
			if (gain < 0) {
				gain = 0;
			} else if (gain > 255) {
				gain = 255;
			}
			if (gain == 255) {
				gain = 256; /* True 100% volume */
			}
			mixer_set_gain(stream, gain);
			break;
		}
		case PLAY: {
			/* Play sound on stream or resume paused stream */
			accept_or_return(PLAY, ctx);
			mixer_stream_t* stream = get_stream(ctx, expr(ctx));
			if (!stream) {
				tokenizer_error_print(ctx, "Invalid STREAM handle");
				return;
			}
			if (tokenizer_token(ctx) == NEWLINE) {
				/* Resume paused stream */
				mixer_resume_stream(stream);
			} else {
				/* Play sound on stream */
				accept_or_return(COMMA, ctx);
				int64_t handle = expr(ctx);
				basic_sound_t *s = (basic_sound_t *)(uintptr_t)handle;
				if (!s || !sound_list_validate(ctx, s)) {
					tokenizer_error_print(ctx, "SOUND PLAY: Invalid sound handle");
					return;
				}
				mixer_push(stream, s->pcm, s->frames);
			}
			break;
		}
		case LOAD: {
			/* Load sound into a sound handle */
			accept_or_return(LOAD, ctx);
			size_t var_length;
			const char* variable = tokenizer_variable_name(ctx, &var_length);
			accept_or_return(VARIABLE, ctx);
			accept_or_return(COMMA, ctx);
			const char* filename = str_expr(ctx);
			basic_sound_t *s = load_sound_from_path(ctx, filename);
			if (!s) {
				return;
			}
			sound_list_add(ctx, s);
			int64_t handle = (int64_t)(uintptr_t)s;
			basic_set_int_variable(variable, handle, ctx, false, false);
			break;
		}
		case UNLOAD: {
			/* Unload sound from a sound handle */
			accept_or_return(UNLOAD, ctx);
			int64_t handle = expr(ctx);
			basic_sound_t *s = (basic_sound_t *)(uintptr_t)handle;
			if (!s || !sound_list_remove(ctx, s)) {
				tokenizer_error_print(ctx, "SOUND UNLOAD: Invalid sound handle");
				return;
			}
			break;
		}
		case STOP: {
			/* Stop stream */
			accept_or_return(STOP, ctx);
			mixer_stream_t* stream = get_stream(ctx, expr(ctx));
			if (!stream) {
				tokenizer_error_print(ctx, "Invalid STREAM handle");
				return;
			}
			mixer_stop_stream(stream);
			break;
		}
		case PAUSE: {
			/* Pause stream */
			accept_or_return(PAUSE, ctx);
			mixer_stream_t* stream = get_stream(ctx, expr(ctx));
			if (!stream) {
				tokenizer_error_print(ctx, "Invalid STREAM handle");
				return;
			}
			mixer_pause_stream(stream);
			break;
		}
		default:
			tokenizer_error_print(ctx, "Expected PLAY, STOP, PAUSE or VOLUME after SOUND");
			return;
	}
	accept_or_return(NEWLINE, ctx);
}

/* Log table mapping dB [-60..0] to [0..255], index 0 == -60 dB, index 60 == 0 dB. */
static const uint8_t db_to_255[61] = {
	0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
	3, 3, 3, 4, 4, 5, 5, 6, 6, 7,
	8, 9, 10, 11, 13, 14, 16, 18, 20, 23,
	26, 29, 32, 36, 40, 45, 51, 57, 64, 72,
	81, 90, 102, 114, 128, 143, 161, 181, 203, 227,
	255
};

int64_t basic_decibels(struct basic_ctx* ctx) {
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("DECIBELS", 0);

	int64_t dB = intval;

	if (dB >= 0) {
		return 255;
	}
	if (dB <= -60) {
		return 0;
	}

	/* Map -60..0 dB to 0..255 via lookup */
	return (int64_t)db_to_255[(int)(dB + 60)];
}

