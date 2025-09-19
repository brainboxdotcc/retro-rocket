#include <kernel.h>

const double PI = 3.141592653589793238;

bool envelope_define(struct basic_ctx *ctx, int idx, tone_wave_t wave, uint8_t volume,  uint8_t pulse_width, uint32_t attack_ms, uint32_t decay_ms, uint8_t sustain, uint32_t release_ms, int32_t vibrato_cents, uint32_t vibrato_hz, uint32_t glide_ms, uint32_t pwm_hz, uint8_t pwm_depth) {
	if (!ctx || idx < 0 || idx >= 64) {
		return false;
	}

	/* avoid pathological duty extremes for square; harmless for other waves */
	if (pulse_width == 0) pulse_width = 1;
	if (pulse_width == 255) pulse_width = 254;

	sound_envelope_ex_t *e = &ctx->envelopes[idx];

	e->in_use = true;
	e->wave = wave;
	e->volume = volume;
	e->pulse_width = pulse_width;

	e->attack_ms = attack_ms;
	e->decay_ms = decay_ms;
	e->sustain = sustain;
	e->release_ms = release_ms;

	e->vibrato_cents = vibrato_cents;
	e->vibrato_hz = vibrato_hz;
	e->glide_ms = glide_ms;

	e->pwm_hz = pwm_hz;
	e->pwm_depth = pwm_depth;

	return true;
}


static bool generate_tone_44100_stereo(struct basic_ctx *ctx, uint32_t freq_hz, uint32_t dur_cs, const sound_envelope_ex_t *env, int16_t **out_pcm, size_t *out_bytes) {
	if (!out_pcm || !out_bytes || freq_hz == 0 || dur_cs == 0) {
		return false;
	}

	const uint32_t rate = 44100;
	const size_t frames = (size_t) ((uint64_t) dur_cs * rate / 100);
	const size_t samples = frames * 2; /* stereo */
	int16_t *pcm = buddy_malloc(ctx->allocator, samples * sizeof(int16_t));
	if (!pcm) {
		return false;
	}

	/* default envelope if none */
	tone_wave_t wave = env ? env->wave : TONE_SQUARE;
	uint8_t volume = env ? env->volume : 255;
	uint8_t sustain = env ? env->sustain : 255;
	uint8_t pulse_width = env ? env->pulse_width : 128;
	uint32_t A = env ? env->attack_ms : 0;
	uint32_t D = env ? env->decay_ms : 5;
	uint32_t R = env ? env->release_ms : 5;

	const size_t Af = (size_t) ((uint64_t) A * rate / 1000);
	const size_t Df = (size_t) ((uint64_t) D * rate / 1000);
	const size_t Rf = (size_t) ((uint64_t) R * rate / 1000);
	const size_t Sf = (frames > Af + Df + Rf) ? (frames - (Af + Df + Rf)) : 0;

	const uint64_t ONE_Q32 = (1ull << 32);
	const uint64_t step_q32 = (uint64_t) ((((long double) freq_hz) / (long double) rate) * (long double) ONE_Q32);
	uint64_t phase_q32 = 0;

	for (size_t i = 0; i < frames; i++) {
		/* envelope progression */
		uint32_t env_level = 255;
		if (i < Af) {
			env_level = (Af ? (uint32_t) ((i * 255ull) / Af) : 255);
		} else if (i < Af + Df) {
			size_t t = i - Af;
			env_level = (uint32_t) (255 - ((uint64_t) (255 - sustain) * t) / (Df ? Df : 1));
		} else if (i < Af + Df + Sf) {
			env_level = sustain;
		} else {
			size_t t = i - (Af + Df + Sf);
			env_level = (t >= Rf) ? 0u : (uint32_t) ((uint64_t) sustain * (Rf - t) / (Rf ? Rf : 1));
		}

		phase_q32 += step_q32;
		double t = (double)(phase_q32 & 0xFFFFFFFFull) / (double)(1ull << 32);


		int32_t s = 0;
		switch (wave) {
			case TONE_SQUARE: {
				int on = (((phase_q32 >> 24) & 0xFF) < pulse_width) ? 1 : 0;
				s = on ? 32767 : -32767;
				break;
			}
			case TONE_TRIANGLE:
				s = (int32_t) ((2.0 * fabs(2.0 * t - 1.0) - 1.0) * 32767);
				break;
			case TONE_SINE:
				s = (int32_t) (sin(2.0 * PI * t) * 32767);
				break;
			case TONE_SAW:
				s = (int32_t) ((2.0 * t - 1.0) * 32767);
				break;
			case TONE_NOISE:
				s = (rand() % 65536) - 32768;
				break;
		}

		/* apply envelope + volume scaling */
		int64_t out = (int64_t) s * env_level * volume;
		out /= (255 * 255);
		if (out > 32767) out = 32767;
		if (out < -32768) out = -32768;

		int16_t v = (int16_t) out;
		pcm[2 * i + 0] = v;
		pcm[2 * i + 1] = v;
	}

	*out_pcm = pcm;
	*out_bytes = samples * sizeof(int16_t);
	return true;
}

bool sound_cmd_tone(struct basic_ctx *ctx, mixer_stream_t *stream, uint32_t freq_hz, uint32_t dur_cs, int env_idx_opt) {
	if (!stream || freq_hz == 0 || dur_cs == 0) {
		return false;
	}

	const sound_envelope_ex_t *env = NULL;
	if (env_idx_opt >= 0 && env_idx_opt < 64 && ctx) {
		sound_envelope_ex_t *e = &ctx->envelopes[env_idx_opt];
		if (e->in_use) {
			env = e;
		}
	}

	int16_t *pcm = NULL;
	size_t bytes = 0;
	if (!generate_tone_44100_stereo(ctx, freq_hz, dur_cs, env, &pcm, &bytes)) {
		return false;
	}

	size_t frames_total = bytes / (2 * sizeof(int16_t));
	size_t taken = mixer_push(stream, pcm, frames_total);
	buddy_free(ctx->allocator, pcm);

	return taken == frames_total;
}
