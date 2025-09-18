// modules/mp3/mp3_loader.c

#include <kernel.h>

/* Map minimp3 allocations to kernel heap just for this include */
#define malloc  kmalloc
#define free    kfree
#define realloc krealloc

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_STDIO
#define MINIMP3_ALLOW_MONO_STEREO_TRANSITION
#define MINIMP3_ONLY_SIMD

#include "minimp3_ex.h"

#undef malloc
#undef free
#undef realloc

static audio_file_loader_t *g_mp3_loader = NULL;

typedef struct {
	uint32_t in_rate;
	uint32_t out_rate;
	uint32_t channels;
	uint32_t step_q16;
	uint32_t pos_q16;
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
		if (frames > out_capacity_frames) {
			frames = out_capacity_frames;
		}
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
			for (uint32_t c = 0; c < ch && c < 8; c++) prev[c] = curr[c];
			in_index++;
		}
		if (in_index >= in_frames) {
			break;
		}

		const int16_t *curr = in_pcm + in_index * ch;
		uint32_t frac = pos & 0xFFFFu;
		uint32_t ifrac = 65536u - frac;

		int16_t *o = out_pcm + out_frames * ch;
		for (uint32_t c = 0; c < ch && c < 8; c++) {
			int32_t a = (int32_t) prev[c] * (int32_t) ifrac;
			int32_t b = (int32_t) curr[c] * (int32_t) frac;
			int32_t s = (a + b) >> 16;
			if (s > 32767) s = 32767;
			if (s < -32768) s = -32768;
			o[c] = (int16_t) s;
		}

		out_frames++;
		pos += step;

		while (pos >= (1u << 16) && in_index < in_frames) {
			pos -= (1u << 16);
			const int16_t *next = in_pcm + in_index * ch;
			for (uint32_t c = 0; c < ch && c < 8; c++) prev[c] = next[c];
			in_index++;
		}
	}

	for (uint32_t c = 0; c < ch && c < 8; c++) {
		st->last_frame[c] = prev[c];
	}
	st->pos_q16 = pos;
	return out_frames;
}

static bool mp3_from_memory(const char *filename, const void *mp3, size_t mp3_bytes, void **out_ptr, size_t *out_bytes) {
	if (!filename || !out_ptr || !out_bytes || !mp3 || mp3_bytes == 0) {
		return false;
	}
	if (!has_suffix_icase(filename, ".mp3")) {
		return false;
	}

	*out_ptr = NULL;
	*out_bytes = 0;

	mp3dec_t d;
	mp3dec_file_info_t finfo;
	memset(&finfo, 0, sizeof(finfo));

	int rc = mp3dec_load_buf(&d, (const unsigned char *) mp3, mp3_bytes, &finfo, NULL, NULL);
	if (rc != 0 || !finfo.buffer || finfo.channels <= 0) {
		kfree(finfo.buffer);
		return false;
	}

	int16_t *stage = (int16_t *) finfo.buffer;
	int stage_ch = finfo.channels;
	int stage_hz = finfo.hz;
	size_t stage_frames = (size_t) finfo.samples / (size_t) stage_ch;

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
		const size_t block = 8192;
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

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("mp3: loaded\n");

	audio_file_loader_t* mp3_loader = kmalloc(sizeof(audio_file_loader_t));
	if (!mp3_loader) {
		dprintf("mp3.ko: failed to allocate loader\n");
		return false;
	}

	mp3_loader->next = NULL;
	mp3_loader->try_load_audio = mp3_from_memory;
	mp3_loader->opaque = NULL;

	if (!register_audio_loader(mp3_loader)) {
		dprintf("mp3: register_audio_loader failed\n");
		kfree(mp3_loader);
		return false;
	}

	g_mp3_loader = mp3_loader;
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	/* Safely remove our loader from the chain and free it */
	if (g_mp3_loader) {
		if (!deregister_audio_loader(g_mp3_loader)) {
			kprintf("mp3: deregister_audio_loader failed\n");
			return false;
		}
		kfree(g_mp3_loader);
		g_mp3_loader = NULL;
	}
	return true;
}
