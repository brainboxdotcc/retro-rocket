/**
 * @file mixer.c
 * @brief Software mixer implementation for Retro Rocket.
 */
#include <kernel.h>
#include <emmintrin.h>

/* Batch size tuned for fewer device calls and good cache behaviour */
#define MIXER_BATCH_FRAMES        16384
#define MIXER_FLUSH_CHUNK_FRAMES  MIXER_BATCH_FRAMES
#define MIXER_MAX_STREAMS   64

/* 32-bit accumulator (interleaved L/R) allocated in mixer_init */
static int32_t *mix_accum = NULL;

typedef struct chunk_t {
	struct chunk_t *next;
	uint32_t frames;     /* total frames stored in this chunk */
	uint32_t rpos;       /* read offset in frames */
	/* samples[] follows (int16_t, interleaved stereo, length = 2 * frames) */
} chunk_t;

struct mixer_stream {
	chunk_t *head;            /* consumer pops from head */
	chunk_t *tail;            /* producer appends to tail */
	uint32_t queued_frames;   /* total frames queued across chunks */
	uint16_t gain_q8_8;       /* 256 == 1.0 */
	bool muted;
	bool paused;
	bool in_use;
	uint32_t chunk_frames;    /* preferred chunk allocation size (frames) */
};

typedef struct {
	audio_device_t *dev;
	struct mixer_stream *streams;
	uint32_t streams_cap;
	uint32_t ring_frames_default;
	uint32_t target_latency_ms;
	uint32_t idle_period_ms;

	/* Scratch mix buffer (interleaved stereo) */
	int16_t *mix_scratch;         /* 2 * MIXER_FLUSH_CHUNK_FRAMES entries */

	/* Idle registration state */
	bool idle_registered;

} mixer_state_t;

static mixer_state_t mix;

static inline uint32_t min_u32(uint32_t a, uint32_t b) {
	if (a < b) {
		return a;
	}
	return b;
}

static inline int16_t clamp_s16(int32_t x) {
	x = CLAMP(x, -326768, 32767);
	return (int16_t)x;
}

static inline size_t chunk_bytes(uint32_t frames)
{
	return sizeof(chunk_t) + sizeof(int16_t) * 2u * frames;
}

/* Append a new chunk of 'frames' copied from src_frames to stream FIFO */
static bool stream_append_chunk(struct mixer_stream *ch, const int16_t *src_frames, uint32_t frames) {
	size_t bytes = chunk_bytes(frames);
	chunk_t *ck = (chunk_t *)kmalloc(bytes);
	if (!ck) {
		return false;
	}

	ck->next = NULL;
	ck->frames = frames;
	ck->rpos = 0;

	int16_t *dst = (int16_t *)(ck + 1);
	memcpy(dst, src_frames, sizeof(int16_t) * 2 * frames);

	if (!ch->tail) {
		ch->head = ck;
		ch->tail = ck;
	} else {
		ch->tail->next = ck;
		ch->tail = ck;
	}

	ch->queued_frames += frames;
	return true;
}

bool mixer_init(audio_device_t* dev, uint32_t target_latency_ms, uint32_t idle_period_ms, uint32_t max_streams) {
	memset(&mix, 0, sizeof(mix));

	mix.dev = dev ? dev : find_first_audio_device();
	if (!mix.dev) {
		dprintf("mixer: Invalid audio device specified\n");
		return false;
	}

	if (max_streams == 0) {
		max_streams = 8;
	}
	if (max_streams > MIXER_MAX_STREAMS) {
		max_streams = MIXER_MAX_STREAMS;
	}

	mix.streams_cap      = max_streams;
	mix.target_latency_ms = target_latency_ms ? target_latency_ms : 60u;  /* safe cushion */
	mix.idle_period_ms    = (idle_period_ms >= 1u) ? idle_period_ms : 1u; /* drive mixer hard */

	/* Channel table */
	size_t table_bytes = sizeof(struct mixer_stream) * mix.streams_cap;
	mix.streams = (struct mixer_stream *)kmalloc(table_bytes);
	if (!mix.streams) {
		memset(&mix, 0, sizeof(mix));
		dprintf("mixer: Failed to allocate streams\n");
		return false;
	}
	memset(mix.streams, 0, table_bytes);

	/* Scratch output (S16 interleaved) */
	size_t s16_samples = 2u * MIXER_BATCH_FRAMES;
	mix.mix_scratch = (int16_t *)kmalloc(sizeof(int16_t) * s16_samples);
	if (!mix.mix_scratch) {
		kfree(mix.streams);
		memset(&mix, 0, sizeof(mix));
		dprintf("mixer: Failed to allocate s16 scratch\n");
		return false;
	}

	/* 32-bit accumulator (interleaved) */
	size_t s32_samples = 2u * MIXER_BATCH_FRAMES;
	mix_accum = (int32_t *)kmalloc(sizeof(int32_t) * s32_samples);
	if (!mix_accum) {
		kfree(mix.mix_scratch);
		kfree(mix.streams);
		memset(&mix, 0, sizeof(mix));
		dprintf("mixer: Failed to allocate s32 accum\n");
		return false;
	}

	/* Register foreground idle to drive the mixer */
	proc_register_idle(mixer_idle, IDLE_FOREGROUND, mix.idle_period_ms);
	mix.idle_registered = true;

	dprintf("mixer: Attached to %s with idle period %u\n", mix.dev->name, mix.idle_period_ms);
	return true;
}

mixer_stream_t *mixer_create_stream(void) {
	if (!mix.streams) {
		return NULL;
	}

	for (uint32_t i = 0; i < mix.streams_cap; i++) {
		struct mixer_stream *ch = &mix.streams[i];
		if (!ch->in_use) {
			ch->head = NULL;
			ch->tail = NULL;
			ch->queued_frames = 0;
			ch->gain_q8_8 = 256u;   /* 1.0 */
			ch->muted = false;
			ch->paused = false;
			ch->in_use = true;
			ch->chunk_frames = 2048u; /* ~42.7 ms @ 48 kHz */
			return ch;
		}
	}

	return NULL;
}

void mixer_pause_stream(mixer_stream_t *ch) {
	if (!ch) {
		return;
	}

	ch->paused = true;
}

void mixer_resume_stream(mixer_stream_t *ch) {
	if (!ch) {
		return;
	}

	ch->paused = false;
}

bool mixer_stream_is_paused(mixer_stream_t* ch) {
	if (!ch) {
		return false;
	}
	return ch->paused;
}

void mixer_stop_stream(mixer_stream_t* ch) {
	/* Drop any queued audio */
	chunk_t *p = ch->head;
	while (p) {
		chunk_t *n = p->next;
		kfree(p);
		p = n;
	}

	ch->head = NULL;
	ch->tail = NULL;
	ch->queued_frames = 0;
}

void mixer_free_stream(mixer_stream_t *ch) {
	if (!ch) {
		return;
	}

	mixer_stop_stream(ch);
	ch->in_use = false;
}

size_t mixer_push(mixer_stream_t *ch, const int16_t *frames, size_t total_frames) {
	if (!ch || !ch->in_use || !frames || total_frames == 0) {
		return 0;
	}

	uint32_t preferred = ch->chunk_frames ? ch->chunk_frames : 2048u;
	uint32_t remaining = (uint32_t)total_frames;
	uint32_t accepted = 0;

	while (remaining > 0) {
		uint32_t batch = remaining > preferred ? preferred : remaining;
		if (!stream_append_chunk(ch, frames + 2u * accepted, batch)) {
			break; /* OOM */
		}
		accepted += batch;
		remaining -= batch;

		/* Optional growth on sustained bursts */
		if (preferred < 8192 && accepted >= (preferred * 4)) {
			preferred <<= 1;
			ch->chunk_frames = preferred;
		}
	}

	return accepted;
}

void mixer_set_gain(mixer_stream_t *ch, uint16_t q8_8_gain)
{
	if (!ch) {
		return;
	}
	ch->gain_q8_8 = q8_8_gain;
}

void mixer_set_mute(mixer_stream_t *ch, bool mute)
{
	if (!ch) {
		return;
	}
	ch->muted = mute;
}

uint32_t mixer_stream_queue_length(mixer_stream_t *ch)
{
	if (!ch) {
		return 0;
	}
	return ch->queued_frames;
}

audio_device_t *mixer_device(void) {
	return mix.dev;
}

void mixer_idle(void)
{
	if (!mix.dev) {
		return;
	}

	const uint32_t rate    = mix.dev->frequency    ? mix.dev->frequency()    : 41000;
	const uint32_t want_ms = mix.target_latency_ms ? mix.target_latency_ms   : 200;
	const uint32_t have_ms = mix.dev->queue_length ? mix.dev->queue_length() : 0;
	const uint32_t safety = 100; /* ms cushion */
	const uint32_t target = want_ms + safety;

	if (have_ms >= target) {
		return;
	}

	uint32_t need_ms = (target + 1) - have_ms;
	uint32_t need_frames = (uint32_t)(((uint64_t)need_ms * (uint64_t)rate) / 1000);
	if (need_frames == 0) {
		return;
	}

	while (need_frames > 0) {
		const uint32_t period = 8192;
		uint32_t batch = need_frames;

		if (batch > MIXER_FLUSH_CHUNK_FRAMES) {
			batch = MIXER_FLUSH_CHUNK_FRAMES;
		}

		/* quantise to whole periods */
		batch = (batch / period) * period;
		if (batch == 0) {
			batch = period;
		}

		/* clear accumulator for this output block */
		memset(mix_accum, 0, sizeof(int32_t) * 2u * batch);

		/* mix stream into this block */
		for (uint32_t c = 0; c < mix.streams_cap; c++) {
			struct mixer_stream *ch = &mix.streams[c];
			if (!ch->in_use || ch->muted || !ch->head || ch->paused) {
				continue;
			}

			uint32_t frames_left = batch;
			uint32_t out_idx     = 0;
			uint16_t gain        = ch->gain_q8_8;

			chunk_t *ck = ch->head;
			while (frames_left > 0 && ck) {
				/* drop exhausted chunks */
				if (ck->rpos >= ck->frames) {
					ch->head = ck->next;
					if (!ch->head) {
						ch->tail = NULL;
					}
					kfree(ck);
					ck = ch->head;
					continue;
				}

				uint32_t avail = ck->frames - ck->rpos;
				uint32_t take  = (avail < frames_left) ? avail : frames_left;

				const int16_t *src = ((const int16_t *)(ck + 1)) + 2u * ck->rpos;
				int32_t *dst       = mix_accum + 2u * out_idx;

				if (gain == 256) {
					/* unity gain fast path */
					for (uint32_t i = 0; i < take; i++) {
						dst[0] += src[0];
						dst[1] += src[1];
						src += 2;
						dst += 2;
					}
				} else {
					__m128i vgain16 = _mm_set1_epi16((int16_t)gain);
					uint32_t i = 0;
					for (; i + 4 <= take; i += 4) {
						__m128i s16   = _mm_loadu_si128((const __m128i *)(src + 2u * i));
						__m128i lo16  = _mm_unpacklo_epi16(s16, _mm_srai_epi16(s16, 15));
						__m128i hi16  = _mm_unpackhi_epi16(s16, _mm_srai_epi16(s16, 15));
						__m128i g_lo  = _mm_madd_epi16(lo16, _mm_unpacklo_epi16(vgain16, vgain16));
						__m128i g_hi  = _mm_madd_epi16(hi16, _mm_unpacklo_epi16(vgain16, vgain16));
						__m128i sc_lo = _mm_srai_epi32(g_lo, 8);
						__m128i sc_hi = _mm_srai_epi32(g_hi, 8);
						__m128i acc0  = _mm_loadu_si128((__m128i *)(dst + 0));
						__m128i acc1  = _mm_loadu_si128((__m128i *)(dst + 4));
						acc0 = _mm_add_epi32(acc0, sc_lo);
						acc1 = _mm_add_epi32(acc1, sc_hi);
						_mm_storeu_si128((__m128i *)(dst + 0), acc0);
						_mm_storeu_si128((__m128i *)(dst + 4), acc1);
						dst += 8;
					}
					for (; i < take; i++) {
						dst[0] += ((int32_t)src[0] * (int32_t)gain) >> 8;
						dst[1] += ((int32_t)src[1] * (int32_t)gain) >> 8;
						src += 2;
						dst += 2;
					}
				}

				ck->rpos += take;
				ch->queued_frames = (ch->queued_frames >= take) ? (ch->queued_frames - take) : 0;

				out_idx     += take;
				frames_left -= take;
			}
		}

		/* convert accumulator to S16 */
		const uint32_t n32 = batch * 2u;
		uint32_t i = 0;
		for (; i + 8 <= n32; i += 8) {
			__m128i a0 = _mm_loadu_si128((const __m128i *)&mix_accum[i + 0]);
			__m128i a1 = _mm_loadu_si128((const __m128i *)&mix_accum[i + 4]);
			__m128i p  = _mm_packs_epi32(a0, a1);
			_mm_storeu_si128((__m128i *)&mix.mix_scratch[i], p);
		}
		for (; i < n32; i++) {
			int32_t x = mix_accum[i];
			x = CLAMP(x, -32768, 32767);
			mix.mix_scratch[i] = (int16_t)x;
		}

		/* push block to device */
		mix.dev->play(mix.mix_scratch, batch);

		/* we just delivered 'batch' frames this tick */
		if (need_frames > batch) {
			need_frames -= batch;
		} else {
			need_frames = 0;
		}

	}
}
