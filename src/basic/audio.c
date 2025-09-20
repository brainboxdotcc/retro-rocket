#include <kernel.h>
#include <emmintrin.h>


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
	if (!audio_file_load(path, &bits, &size)) {
		tokenizer_error_printf(ctx, "Unable to load audio file '%s'", path);
		return NULL;
	}
	basic_sound_t* sound = buddy_malloc(ctx->allocator, sizeof(basic_sound_t));
	if (!sound) {
		tokenizer_error_printf(ctx, "Out of memory loading audio file '%s'", path);
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

void envelope_statement(struct basic_ctx* ctx) {
	if (!find_first_audio_device()) {
		tokenizer_error_print(ctx, "ENVELOPE: No sound driver is loaded");
		return;
	}
	accept_or_return(ENVELOPE, ctx);
	switch (tokenizer_token(ctx)) {
		case CREATE: {
			/* Create new envelope */
			accept_or_return(CREATE, ctx);
			int idx = expr(ctx); accept_or_return(COMMA, ctx);
			tone_wave_t wave = (tone_wave_t)expr(ctx); accept_or_return(COMMA, ctx);
			uint8_t volume = expr(ctx); accept_or_return(COMMA, ctx);
			uint8_t pulse_width = expr(ctx); accept_or_return(COMMA, ctx);
			uint32_t attack_ms = expr(ctx); accept_or_return(COMMA, ctx);
			uint32_t decay_ms = expr(ctx); accept_or_return(COMMA, ctx);
			uint8_t sustain = expr(ctx); accept_or_return(COMMA, ctx);
			uint32_t release_ms = expr(ctx); accept_or_return(COMMA, ctx);
			int32_t vibrato_cents = expr(ctx); accept_or_return(COMMA, ctx);
			uint32_t vibrato_hz = expr(ctx); accept_or_return(COMMA, ctx);
			uint32_t glide_ms = expr(ctx); accept_or_return(COMMA, ctx);
			uint32_t pwm_hz = expr(ctx); accept_or_return(COMMA, ctx);
			uint8_t pwm_depth = expr(ctx);
			if (!envelope_define(ctx, idx, wave, volume, pulse_width, attack_ms, decay_ms, sustain, release_ms, vibrato_cents, vibrato_hz, glide_ms, pwm_hz, pwm_depth)) {
				tokenizer_error_print(ctx, "ENVELOPE number out of range");
			}
			break;
		}
		case DESTROY: {
			/* Delete envelope */
			accept_or_return(DESTROY, ctx);
			int64_t envelope_id = expr(ctx);
			if (envelope_id < 0 || envelope_id > 63) {
				tokenizer_error_print(ctx, "Invalid ENVELOPE number");
			}
			ctx->envelopes[envelope_id].in_use = false;
			break;
		}
		default:
			tokenizer_error_print(ctx, "Expected CREATE or DESTROY after ENVELOPE");
			return;
	}
	accept_or_return(NEWLINE, ctx);
}

/* Blocked linear resampler with Hz pitch offset (Q15 weights, SSE2).
   - Input:  interleaved S16LE stereo at 44.1 kHz (immutable)
   - Output: interleaved S16LE stereo at 44.1 kHz
   - maths:  L = (L0*inv + L1*frac) >> 15, same for R (Q15 weights)
*/
static size_t sound_push_with_pitch(struct basic_ctx *ctx, mixer_stream_t *stream, const int16_t *pcm_in, size_t frames_in, int64_t pitch_offset_hz) {
	if (!pitch_offset_hz) {
		return mixer_push(stream, pcm_in, frames_in);
	}

	size_t rv = 0;
	const double base_rate = 44100.0;
	const double new_rate  = base_rate + (double)pitch_offset_hz;
	if (new_rate <= 100.0) {
		tokenizer_error_print(ctx, "SOUND PLAY: Invalid pitch offset");
		return 0;
	}

	/* Advance per output frame in Q16.16 source-frame units. */
	const double   ratio_f = new_rate / base_rate;      /* >1 = faster (higher pitch) */
	const uint32_t STEP_Q16 = (uint32_t)(ratio_f * 65536.0);

	if (frames_in < 2) {
		return 0; /* need at least 2 source frames for lerp */
	}

	const size_t last_valid = frames_in - 2; /* we read idx and idx+1 */
	uint64_t pos_q16 = 0;

	enum { BLOCK_FRAMES = 32768 }; /* ~128 KiB scratch (stereo S16) */
	int16_t *scratch = buddy_malloc(ctx->allocator, (size_t)BLOCK_FRAMES * 2u * sizeof(int16_t));
	if (!scratch) {
		tokenizer_error_print(ctx, "Out of memory for pitch shift");
		return 0;
	}

	while (1) {
		size_t produced = 0;

		while (produced < (size_t)BLOCK_FRAMES) {
			const uint32_t frac_q16 = (uint32_t)(pos_q16 & 0xFFFFu);
			const size_t   idx      = (size_t)(pos_q16 >> 16);
			if (idx > last_valid) {
				break;
			}

			/* Convert Q16.16 -> Q15 for SSE2 signed 16-bit weights. */
			const uint16_t frac_q15 = (uint16_t)(frac_q16 >> 1);          /* 0..32767 */
			const uint16_t inv_q15  = (uint16_t)(32767u - frac_q15);      /* 0..32767 */

			/* Neighbour samples */
			const int16_t L0 = pcm_in[(idx    )*2 + 0];
			const int16_t R0 = pcm_in[(idx    )*2 + 1];
			const int16_t L1 = pcm_in[(idx + 1)*2 + 0];
			const int16_t R1 = pcm_in[(idx + 1)*2 + 1];

			/* vals = [ R1, R0, L1, L0, 0,0,0,0 ] (int16)
			   wts  = [  f,  i,  f,  i, 0,0,0,0 ] (int16, Q15)
			   madd: [ R1*f + R0*i, L1*f + L0*i, 0, 0 ] (int32) */
			const __m128i vals = _mm_set_epi16(0,0,0,0, R1, R0, L1, L0);
			const __m128i wts  = _mm_set_epi16(0,0,0,0, (short)frac_q15, (short)inv_q15,
							   (short)frac_q15, (short)inv_q15);
			__m128i acc = _mm_madd_epi16(vals, wts);    /* two 32-bit sums in low lanes */
			acc = _mm_srai_epi32(acc, 15);              /* >>15 to undo Q15 */

			/* Extract the two 32-bit results via a store (portable SSE2) */
			int32_t tmp[4];
			_mm_storeu_si128((__m128i*)tmp, acc);
			const int32_t R = tmp[0];
			const int32_t L = tmp[1];

			/* Store clamped */
			scratch[produced*2 + 0] = (int16_t)((L > 32767) ? 32767 : (L < -32768 ? -32768 : L));
			scratch[produced*2 + 1] = (int16_t)((R > 32767) ? 32767 : (R < -32768 ? -32768 : R));

			produced++;
			pos_q16 += STEP_Q16;
		}

		if (!produced) {
			break;
		}

		rv = mixer_push(stream, scratch, produced);

		if ((size_t)(pos_q16 >> 16) > last_valid) {
			break; /* source consumed */
		}
	}

	buddy_free(ctx->allocator, scratch);
	return rv;
}

void sound_statement(struct basic_ctx* ctx) {
	if (!find_first_audio_device()) {
		tokenizer_error_print(ctx, "SOUND: No sound driver is loaded");
		return;
	}
	accept_or_return(SOUND, ctx);
	switch (tokenizer_token(ctx)) {
		case TONE: {
			/* Set volume for stream */
			accept_or_return(TONE, ctx);
			mixer_stream_t* stream = get_stream(ctx, expr(ctx));
			accept_or_return(COMMA, ctx);
			int64_t freq_hz = expr(ctx);
			accept_or_return(COMMA, ctx);
			int64_t duration_centiseconds = expr(ctx);
			int64_t env_idx_opt = -1;
			if (tokenizer_token(ctx) == COMMA) {
				accept_or_return(COMMA, ctx);
				env_idx_opt = expr(ctx);
			}
			if (!stream) {
				tokenizer_error_print(ctx, "Invalid STREAM handle");
				return;
			}
			if (!sound_cmd_tone(ctx, stream, freq_hz, duration_centiseconds, env_idx_opt)) {
				tokenizer_error_print(ctx, "Out of memory for SOUND TONE");
			}
			break;
		}
		case VOLUME: {
			/* Set volume for stream */
			accept_or_return(VOLUME, ctx);
			mixer_stream_t* stream = get_stream(ctx, expr(ctx));
			if (!stream) {
				tokenizer_error_print(ctx, "Invalid STREAM handle");
				return;
			}
			accept_or_return(COMMA, ctx);
			int64_t gain = CLAMP(expr(ctx), 0, 255);
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
				int64_t pitch_offset_hz = 0;
				if (tokenizer_token(ctx) == COMMA) {
					/* Optional pitch offset */
					accept_or_return(COMMA, ctx);
					pitch_offset_hz = expr(ctx);
				}

				if (pitch_offset_hz == 0) {
					mixer_push(stream, s->pcm, s->frames);
				} else {
					sound_push_with_pitch(ctx, stream, s->pcm, s->frames, pitch_offset_hz);
				}
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

