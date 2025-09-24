/**
 * @file xm.c
 * @brief Retro Rocket XM codec module (via libxm)
 */

#include <kernel.h>
#include "xm.h"

#define XM_TARGET_RATE 44100
#define MAX_TOTAL_FRAMES (XM_TARGET_RATE * 60 * 10) /* safety cap: 10 minutes */

/* state wrapper around libxm */
typedef struct {
	xm_context_t *ctx;
	bool ended;
} xm_state_t;

static inline int16_t f32_to_s16(float v) {
	v = CLAMP(v, -1.0f, 1.0f);
	return (int16_t)(v * 32767.0f);
}

static audio_file_loader_t *g_xm_loader = NULL;

/* convert interleaved float [-1,1] to interleaved s16 */
static inline void f32_to_s16i(const float *src_lr, int16_t *dst_lr, size_t frames) {
	for (size_t i = 0; i < frames * 2; i++) {
		float v = src_lr[i] * 32767.0f;
		if (v > 32767.0f) v = 32767.0f;
		if (v < -32768.0f) v = -32768.0f;
		dst_lr[i] = (int16_t)v;
	}
}

/* xm_from_memory: decode XM to interleaved S16_LE stereo @ 44100 Hz */
static bool xm_from_memory(const char *filename, const void *bytes, size_t len,
			   void **out_ptr, size_t *out_bytes)
{
	dprintf("xm_from_memory\n");
	if (!filename || !bytes || len == 0 || !out_ptr || !out_bytes) {
		dprintf("args\n");
		return false;
	}
	*out_ptr = NULL;
	*out_bytes = 0;

	/* honour the same contract as the MOD loader */
	if (!has_suffix_icase(filename, ".xm") && !has_suffix_icase(filename, ".s3m")) {
		dprintf("suffix\n");
		return false;
	}

	/* --- prescan (opaque size provided by XM_PRESCAN_DATA_SIZE) --- */
	void *prescan_buf = kmalloc(XM_PRESCAN_DATA_SIZE);
	if (!prescan_buf) return false;
	xm_prescan_data_t *p = (xm_prescan_data_t *)prescan_buf;

	if (!xm_prescan_module((const char *)bytes, (uint32_t)len, p)) {
		dprintf("!prescan\n");
		kfree(prescan_buf);
		return false;
	}

	/* --- context allocation & creation --- */
	const uint32_t need = xm_size_for_context(p);
	void *pool = kmalloc(need);
	if (!pool) {
		dprintf("oom pool\n");
		kfree(prescan_buf);
		return false;
	}

	xm_context_t *ctx = xm_create_context((char *)pool, p, (const char *)bytes, (uint32_t)len);
	/* prescan no longer needed */
	kfree(prescan_buf);

	if (!ctx) {
		dprintf("oom ctx\n");
		kfree(pool);
		return false;
	}

	/* fixed output rate per Retro Rocket contract */
	xm_set_sample_rate(ctx, XM_TARGET_RATE); /* XM_TARGET_RATE == 44100 */
	xm_set_max_loop_count(ctx, 1);                    /* play one full loop */
	xm_reset_context(ctx);

	uint16_t mod_len = xm_get_module_length(ctx);
	dprintf("order length=%u\n", mod_len);

	/* --- output PCM (S16_LE, interleaved stereo) --- */
	size_t cap_frames  = 1u << 16; /* start with 65,536 frames */
	size_t used_frames = 0;
	int16_t *pcm = kmalloc(cap_frames * 2 * sizeof(int16_t));
	if (!pcm) {
		dprintf("oom pcm\n");
		kfree(pool);
		return false;
	}

	/* libxm produces float L/R; render in chunks */
	const size_t chunk_frames = 2048;
	float *scratch = kmalloc(chunk_frames * 2 * sizeof(float));
	if (!scratch) {
		dprintf("oom scratch\n");
		kfree(pcm);
		kfree(pool);
		return false;
	}

	/* safety cap: 10 minutes */
	const size_t max_frames = (size_t)XM_TARGET_RATE * 60u * 10u;
	size_t n = 0;

	dprintf("max frames: %lu\n", max_frames);

	for (uint8_t i = 1; i <= xm_get_number_of_instruments(ctx); i++) {
		uint8_t ns = xm_get_number_of_samples(ctx, i);
		for (uint8_t s = 0; s < ns; s++) {
			uint32_t len = 0;
			xm_sample_point_t *wave = xm_get_sample_waveform(ctx, i, s, &len);
			dprintf("inst %u sample %u len=%u wave=%p\n", i, s, len, wave);
		}
	}

	for (uint8_t ch = 1; ch <= xm_get_number_of_channels(ctx); ch++) {
		xm_mute_channel(ctx, ch, false);
	}

	dprintf("channels=%u patterns=%u instruments=%u\n",
		xm_get_number_of_channels(ctx),
		xm_get_number_of_patterns(ctx),
		xm_get_number_of_instruments(ctx));

	uint8_t bpm = 0, tempo = 0;
	xm_get_playing_speed(ctx, &bpm, &tempo);
	dprintf("bpm=%u tempo=%u\n", bpm, tempo);

	/* --- render loop: generate until first loop or cap --- */
	while (used_frames < max_frames) {
		//dprintf("Iterate %lu\n", n++);
		/* grow output buffer if needed */
		if (used_frames + chunk_frames > cap_frames) {
			size_t new_cap = cap_frames * 2;
			if (new_cap < used_frames + chunk_frames)
				new_cap = used_frames + chunk_frames;
			int16_t *grown = (int16_t *)krealloc(pcm, new_cap * 2 * sizeof(int16_t));
			if (!grown) {
				kfree(scratch);
				kfree(pcm);
				kfree(pool);
				return false;
			}
			pcm = grown;
			cap_frames = new_cap;
		}

		/* synthesise one chunk (float LR interleaved) */
		xm_generate_samples(ctx, scratch, (uint16_t)chunk_frames);

		/* convert float [-1,1] -> S16_LE with CLAMP(), write interleaved */
		int16_t *dst = pcm + (used_frames * 2);
		for (size_t i = 0; i < (chunk_frames * 2); i++) {
			/* scale then clamp to 16-bit range */
			int v = (int)(scratch[i] * 32767.0f);
			dst[i] = (int16_t)CLAMP(v, -32768, 32767);
			/*if (scratch[i]) {
				char val[32];
				double_to_string(scratch[i], val, 32, 5);
				dprintf("%s->%d\n", val, dst[i]);
			}*/
		}

		used_frames += chunk_frames;

		/* stop after first loop (single playthrough) */
		if (xm_get_loop_count(ctx) > 0) {
			dprintf("loop count > 0\n");
			break;
		}

		/*float vol = xm_get_volume_of_channel(ctx, 1);
		float pan = xm_get_panning_of_channel(ctx, 1);
		float freq = xm_get_frequency_of_channel(ctx, 1);
		int vol_i  = (int)(vol  * 1000.0f);
		int pan_i  = (int)(pan  * 1000.0f);
		int freq_i = (int)freq;
		if (vol_i) {
			dprintf("ch1: vol=%d pan=%d freq=%d\n", vol_i, pan_i, freq_i);
		}*/

	}

	uint8_t pat_idx = 0, pat = 0, row = 0;
	uint32_t samples = 0;
	xm_get_position(ctx, &pat_idx, &pat, &row, &samples);
	dprintf("pat_idx=%u pat=%u row=%u samples=%u\n", pat_idx, pat, row, samples);

	/* context memory belongs to us */
	kfree(pool);
	kfree(scratch);

	if (used_frames == 0) {
		kfree(pcm);
		dprintf("empty result\n");
		return false;
	}

	/* trim to fit */
	const size_t out_sz = used_frames * 2 * sizeof(int16_t);
	int16_t *shrink = krealloc(pcm, out_sz);
	if (shrink) pcm = shrink;

	*out_ptr   = pcm;
	*out_bytes = out_sz;
	return true;
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("xm: loaded\n");

	audio_file_loader_t *loader = (audio_file_loader_t*) kmalloc(sizeof(audio_file_loader_t));
	if (!loader) {
		return false;
	}

	loader->next = NULL;
	loader->try_load_audio = xm_from_memory;
	loader->opaque = NULL;

	if (!register_audio_loader(loader)) {
		kfree(loader);
		return false;
	}

	g_xm_loader = loader;
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	if (g_xm_loader) {
		if (!deregister_audio_loader(g_xm_loader)) {
			return false;
		}
		kfree(g_xm_loader);
		g_xm_loader = NULL;
	}
	return true;
}
