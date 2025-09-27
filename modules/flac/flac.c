#include <kernel.h>

#define DRFLAC_MALLOC(sz) kmalloc(sz)
#define DRFLAC_REALLOC(p, sz) krealloc(p,sz)
#define DRFLAC_FREE(p) kfree(p)
#define DRFLAC_ASSERT(x) {}
#define DR_FLAC_NO_WCHAR
#define DR_FLAC_NO_STDIO
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

typedef struct {
	uint32_t in_rate;
	uint32_t out_rate;
	uint32_t channels;
	uint32_t step_q16;     /* (in_rate << 16) / out_rate */
	uint32_t pos_q16;      /* fractional phase */
	int16_t last_frame[8];
	int has_last;
} resample_lin16_t;

static void resample_lin16_init(resample_lin16_t *st, uint32_t in_rate, uint32_t out_rate, uint32_t channels) {
	st->in_rate = in_rate;
	st->out_rate = out_rate;
	st->channels = channels;
	st->step_q16 = (out_rate ? (uint32_t) (((uint64_t) in_rate << 16) / out_rate) : 0);
	st->pos_q16 = 0;
	st->has_last = 0;
	for (uint32_t c = 0; c < 8; c++) {
		st->last_frame[c] = 0;
	}
}

static size_t resample_lin16_process(resample_lin16_t *st, const int16_t *in_pcm, size_t in_frames, int16_t *out_pcm, size_t out_capacity_frames) {
	if (st->in_rate == st->out_rate) {
		size_t frames = in_frames;
		if (frames > out_capacity_frames) frames = out_capacity_frames;
		size_t samples = frames * st->channels;
		if (samples) {
			memcpy(out_pcm, in_pcm, samples * sizeof(int16_t));
			const int16_t *tail = in_pcm + (frames - 1) * st->channels;
			for (uint32_t c = 0; c < st->channels && c < 8; c++) {
				st->last_frame[c] = tail[c];
			}
			st->has_last = 1;
		}
		return frames;
	}

	const uint32_t ch = st->channels;
	const uint32_t step = st->step_q16;
	uint32_t pos = st->pos_q16;

	size_t out_frames = 0;
	size_t in_index = 0;

	int16_t prev[8];
	if (st->has_last) {
		for (uint32_t c = 0; c < ch && c < 8; c++) {
			prev[c] = st->last_frame[c];
		}
	} else if (in_frames > 0) {
		const int16_t *first = in_pcm;
		for (uint32_t c = 0; c < ch && c < 8; c++) {
			prev[c] = first[c];
		}
		for (uint32_t c = 0; c < ch && c < 8; c++) {
			st->last_frame[c] = prev[c];
		}
		st->has_last = 1;
	} else {
		return 0;
	}

	while (out_frames < out_capacity_frames) {
		while (pos >= (1u << 16) && in_index < in_frames) {
			pos -= (1u << 16);
			const int16_t *curr = in_pcm + in_index * ch;
			for (uint32_t c = 0; c < ch && c < 8; c++) {
				prev[c] = curr[c];
			}
			in_index++;
		}
		if (in_index >= in_frames) {
			break;
		}

		const int16_t *curr = in_pcm + in_index * ch;
		uint32_t frac = pos & 0xFFFFu;     /* 0..65535 */
		uint32_t ifrac = 65536u - frac;

		int16_t *o = out_pcm + out_frames * ch;
		for (uint32_t c = 0; c < ch && c < 8; c++) {
			int32_t a = (int32_t) prev[c] * (int32_t) ifrac;
			int32_t b = (int32_t) curr[c] * (int32_t) frac;
			int32_t s = (a + b) >> 16;
			s = CLAMP(s, -32768, 32768);
			o[c] = (int16_t) s;
		}

		out_frames++;
		pos += step;

		while (pos >= (1u << 16) && in_index < in_frames) {
			pos -= (1u << 16);
			const int16_t *next = in_pcm + in_index * ch;
			for (uint32_t c = 0; c < ch && c < 8; c++) {
				prev[c] = next[c];
			}
			in_index++;
		}
	}

	for (uint32_t c = 0; c < ch && c < 8; c++) {
		st->last_frame[c] = prev[c];
	}
	st->pos_q16 = pos;
	return out_frames;
}

static bool flac_from_memory(const char *filename, const void *flac, size_t flac_bytes, void **out_ptr, size_t *out_bytes) {
	if (!filename || !out_ptr || !out_bytes || !flac || flac_bytes == 0) {
		return false;
	}
	if (!has_suffix_icase(filename, ".flac")) {
		return false;
	}

	*out_ptr = NULL;
	*out_bytes = 0;

	unsigned int ch = 0, hz = 0;
	drflac_uint64 frames = 0;

	int16_t *stage = drflac_open_memory_and_read_pcm_frames_s16(flac, flac_bytes, &ch, &hz, &frames, NULL);
	if (!stage || ch == 0) {
		kfree(stage);
		return false;
	}

	size_t stage_frames = (size_t) frames;
	int stage_ch = (int) ch;
	int stage_hz = (int) hz;

	/* Resample to 44.1 kHz if needed (channels unchanged here). */
	if (stage_hz != 44100) {
		size_t dst_frames_est = (size_t) (((uint64_t) stage_frames * 44100u + (uint64_t) stage_hz - 1u) / (uint64_t) stage_hz);
		int16_t *dst = kmalloc(dst_frames_est * (size_t) stage_ch * sizeof(int16_t));
		if (!dst) {
			kfree(stage);
			return false;
		}

		resample_lin16_t rs;
		resample_lin16_init(&rs, (uint32_t) stage_hz, 44100u, (uint32_t) stage_ch);

		size_t written = 0;
		const size_t block = 8192; /* frames per chunk at source rate */
		for (size_t i = 0; i < stage_frames; i += block) {
			size_t n = stage_frames - i;
			if (n > block) n = block;
			size_t prod = resample_lin16_process(&rs, stage + i * (size_t) stage_ch, n, dst + written * (size_t) stage_ch, dst_frames_est - written);
			written += prod;
		}

		kfree(stage);
		stage = dst;
		stage_frames = written;
		stage_hz = 44100;
	}

	/* Upmix mono → stereo to meet engine invariant. */
	if (stage_ch == 1) {
		int16_t *stereo = kmalloc(stage_frames * 2 * sizeof(int16_t));
		if (!stereo) {
			kfree(stage);
			return false;
		}
		for (size_t i = 0; i < stage_frames; i++) {
			int16_t s = stage[i];
			stereo[2 * i] = s;
			stereo[2 * i + 1] = s;
		}
		kfree(stage);
		stage = stereo;
		stage_ch = 2;
	}

	*out_ptr = stage;
	*out_bytes = stage_frames * 2 * sizeof(int16_t);
	return true;
}

static audio_file_loader_t *g_flac_loader = NULL;

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("flac: loaded\n");

	audio_file_loader_t *l = kmalloc(sizeof(audio_file_loader_t));
	if (!l) {
		dprintf("flac: failed to allocate loader\n");
		return false;
	}

	l->next = NULL;
	l->try_load_audio = flac_from_memory;
	l->opaque = NULL;

	if (!register_audio_loader(l)) {
		dprintf("flac: register_audio_loader failed\n");
		kfree(l);
		return false;
	}

	g_flac_loader = l;
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	if (g_flac_loader) {
		if (!deregister_audio_loader(g_flac_loader)) {
			kprintf("flac: deregister_audio_loader failed\n");
			return false;
		}
		kfree(g_flac_loader);
		g_flac_loader = NULL;
	}
	return true;
}
