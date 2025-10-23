#define STB_VORBIS_NO_CRT
#define STB_VORBIS_NO_STDIO

#include <kernel.h>
#define free(x) kfree(x)

#include "stb_vorbis.h"

/* Stupid stb_vorbis defines these and leaves them defined! */
#undef L
#undef R
#undef C

typedef struct {
	uint32_t in_rate;
	uint32_t out_rate;
	uint32_t channels;
	uint32_t step_q16;     /* (in_rate << 16) / out_rate */
	uint32_t pos_q16;      /* fractional phase 0..65535 */
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
	for (uint32_t c = 0; c < 8; c++) st->last_frame[c] = 0;
}

static size_t resample_lin16_process(resample_lin16_t *st, const int16_t *in_pcm, size_t in_frames, int16_t *out_pcm, size_t out_capacity_frames) {
	if (st->in_rate == st->out_rate) {
		size_t frames = in_frames;
		if (frames > out_capacity_frames) frames = out_capacity_frames;
		size_t samples = frames * st->channels;
		if (samples) {
			memcpy(out_pcm, in_pcm, samples * sizeof(int16_t));
			const int16_t *tail = in_pcm + (frames ? (frames - 1) * st->channels : 0);
			for (uint32_t c = 0; c < st->channels && c < 8; c++) st->last_frame[c] = tail[c];
			st->has_last = (frames != 0);
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
		for (uint32_t c = 0; c < ch && c < 8; c++) prev[c] = st->last_frame[c];
	} else if (in_frames > 0) {
		const int16_t *first = in_pcm;
		for (uint32_t c = 0; c < ch && c < 8; c++) prev[c] = first[c];
		for (uint32_t c = 0; c < ch && c < 8; c++) st->last_frame[c] = prev[c];
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
		if (in_index >= in_frames) break;

		const int16_t *curr = in_pcm + in_index * ch;
		uint32_t frac = pos & 0xFFFFu;      /* 0..65535 */
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

	for (uint32_t c = 0; c < ch && c < 8; c++) st->last_frame[c] = prev[c];
	st->pos_q16 = pos;
	return out_frames;
}

static bool vorbis_from_memory(const char *filename, const void *data, size_t bytes, void **out_ptr, size_t *out_bytes) {
	if (!filename || !out_ptr || !out_bytes || !data || bytes == 0) return false;
	if (!(has_suffix_icase(filename, ".ogg") || has_suffix_icase(filename, ".oga"))) return false;

	*out_ptr = NULL;
	*out_bytes = 0;

	int error = 0;
	stb_vorbis *v = stb_vorbis_open_memory((const unsigned char *) data, (int) bytes, &error, NULL);
	if (!v) return false;

	stb_vorbis_info info = stb_vorbis_get_info(v);
	int src_ch = info.channels;
	int src_hz = info.sample_rate;
	if (src_ch <= 0) {
		stb_vorbis_close(v);
		return false;
	}

	/* Decode whole file in chunks, growing a single buffer. */
	const int ch = src_ch;
	const int chunk_frames = 4096;
	int16_t *pcm = NULL;
	size_t frames_cap = 0;
	size_t frames_len = 0;

	for (;;) {
		/* ensure capacity for next chunk */
		if (frames_len + (size_t) chunk_frames > frames_cap) {
			size_t new_cap = frames_cap ? (frames_cap * 2) : (size_t) 16384;
			if (new_cap < frames_len + (size_t) chunk_frames) new_cap = frames_len + (size_t) chunk_frames;
			int16_t *npcm = krealloc(pcm, new_cap * (size_t) ch * sizeof(int16_t));
			if (!npcm) {
				if (pcm) kfree(pcm);
				stb_vorbis_close(v);
				return false;
			}
			pcm = npcm;
			frames_cap = new_cap;
		}

		int to_read_samples = chunk_frames * ch; /* in s16 samples */
		int got_per_channel = stb_vorbis_get_samples_short_interleaved(v, ch,
									       pcm + frames_len * (size_t) ch, to_read_samples);
		if (got_per_channel <= 0) break; /* EOF */
		frames_len += (size_t) got_per_channel;
	}

	stb_vorbis_close(v);

	if (frames_len == 0) {
		if (pcm) kfree(pcm);
		return false;
	}

	int16_t *stage = pcm;           /* s16 interleaved @ source rate */
	size_t stage_frames = frames_len;
	int stage_ch = src_ch;
	int stage_hz = src_hz;

	/* Downmix >2 channels (rare): simple average to stereo. */
	if (stage_ch > 2) {
		int16_t *stereo = kmalloc(stage_frames * 2 * sizeof(int16_t));
		if (!stereo) {
			kfree(stage);
			return false;
		}
		for (size_t i = 0; i < stage_frames; i++) {
			const int16_t *in = stage + i * (size_t) stage_ch;
			int64_t accL = 0, accR = 0;
			for (int c = 0; c < stage_ch; c++) {
				if ((c & 1) == 0) accL += in[c]; else accR += in[c];
			}
			int16_t L = (int16_t) (accL / (stage_ch - (stage_ch > 1)));
			int16_t R = (int16_t) (accR / (stage_ch - (stage_ch > 1)));
			stereo[2 * i + 0] = L;
			stereo[2 * i + 1] = R;
		}
		kfree(stage);
		stage = stereo;
		stage_ch = 2;
	}

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
			size_t prod = resample_lin16_process(&rs,
							     stage + i * (size_t) stage_ch, n,
							     dst + written * (size_t) stage_ch,
							     dst_frames_est - written);
			written += prod;
		}

		kfree(stage);
		stage = dst;
		stage_frames = written;
		stage_hz = 44100;
	}

	/* Upmix mono → stereo. */
	if (stage_ch == 1) {
		int16_t *stereo = kmalloc(stage_frames * 2 * sizeof(int16_t));
		if (!stereo) {
			kfree(stage);
			return false;
		}
		for (size_t i = 0; i < stage_frames; i++) {
			int16_t s = stage[i];
			stereo[2 * i + 0] = s;
			stereo[2 * i + 1] = s;
		}
		kfree(stage);
		stage = stereo;
		stage_ch = 2;
	}

	*out_ptr = stage;                                /* caller kfree()s */
	*out_bytes = stage_frames * 2 /*stereo*/ * sizeof(int16_t);
	return true;
}

static audio_file_loader_t *g_ogg_loader = NULL;

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("ogg: loaded\n");

	audio_file_loader_t *l = kmalloc(sizeof(audio_file_loader_t));
	if (!l) {
		dprintf("ogg: failed to allocate loader\n");
		return false;
	}

	l->next = NULL;
	l->try_load_audio = vorbis_from_memory;
	l->opaque = NULL;

	if (!register_audio_loader(l)) {
		dprintf("ogg: register_audio_loader failed\n");
		kfree(l);
		return false;
	}

	g_ogg_loader = l;
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	/* Safely remove our loader; refuse to unload if unlink fails. */
	if (g_ogg_loader) {
		if (!deregister_audio_loader(g_ogg_loader)) {
			kprintf("ogg: deregister_audio_loader failed\n");
			return false;
		}
		kfree(g_ogg_loader);
		g_ogg_loader = NULL;
	}
	return true;
}

