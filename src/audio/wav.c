/**
 * @file wav.c
 * @author Craig Edwards
 * @copyright Copyright (c) 2012–2025
 * @brief Translate in-memory PCM WAV to 44100 Hz, stereo, S16_LE.
 */

#include <kernel.h>

typedef struct __attribute__((packed)) {
	uint16_t format_tag;
	uint16_t channels;
	uint32_t samplerate;
	uint32_t byterate;
	uint16_t block_align;
	uint16_t bits_per_sample;
} wav_fmt_t;

/* Decode one sample (one channel) at index "n" from a PCM frame buffer.
 * Supports PCM integer (8/16/24/32) and IEEE float (32/64).
 * Returns a signed 16-bit-range value widened to int32_t.
 */
static int32_t decode_sample_to_s16(const uint8_t *base, uint16_t bits, size_t n, bool is_float) {
	if (!is_float) {
		switch (bits) {
			case 8: {
				uint8_t u = base[n];
				int32_t s = (int32_t) u - 128;
				return s << 8;
			}
			case 16: {
				const int16_t *p16 = (const int16_t *) (base + n * 2);
				int16_t s = *p16;
				return (int32_t) s;
			}
			case 24: {
				const uint8_t *p = base + n * 3;
				int32_t v = (int32_t) ((p[0]) | (p[1] << 8) | (p[2] << 16));
				if (v & 0x00800000) {
					v |= 0xFF000000; /* sign-extend 24 -> 32 */
				}
				return v >> 8;
			}
			case 32: {
				const int32_t *p32 = (const int32_t *) (base + n * 4);
				int32_t v = *p32;
				return v >> 16;
			}
			default: {
				return 0;
			}
		}
	} else {
		if (bits == 32) {
			const float *pf = (const float *) (base + n * 4);
			float f = *pf;
			if (f > 1.0f) {
				f = 1.0f;
			}
			if (f < -1.0f) {
				f = -1.0f;
			}
			int32_t s = (int32_t) (f * 32767.0f);
			return s;
		} else if (bits == 64) {
			const double *pd = (const double *) (base + n * 8);
			double d = *pd;
			if (d > 1.0) {
				d = 1.0;
			}
			if (d < -1.0) {
				d = -1.0;
			}
			int32_t s = (int32_t) (d * 32767.0);
			return s;
		} else {
			return 0;
		}
	}
}

/* Read an interleaved frame (all channels) from source, downmix to stereo LR in S16 range. */
static void fetch_frame_lr_s16(const uint8_t *frame_base, uint16_t channels, uint16_t bits, bool is_float, int32_t *out_l, int32_t *out_r) {
	if (channels == 1) {
		int32_t m = decode_sample_to_s16(frame_base, bits, 0, is_float);
		*out_l = m;
		*out_r = m;
		return;
	}

	if (channels == 2) {
		int32_t l = decode_sample_to_s16(frame_base, bits, 0, is_float);
		int32_t r = decode_sample_to_s16(frame_base, bits, 1, is_float);
		*out_l = l;
		*out_r = r;
		return;
	}

	uint32_t ch;
	int64_t acc_l = 0;
	int64_t acc_r = 0;
	uint32_t cnt_l = 0;
	uint32_t cnt_r = 0;

	for (ch = 0; ch < channels; ch++) {
		int32_t s = decode_sample_to_s16(frame_base, bits, ch, is_float);
		if ((ch & 1u) == 0u) {
			acc_l += s;
			cnt_l++;
		} else {
			acc_r += s;
			cnt_r++;
		}
	}

	if (cnt_l == 0) {
		cnt_l = 1;
	}
	if (cnt_r == 0) {
		cnt_r = 1;
	}

	*out_l = (int32_t) (acc_l / (int64_t) cnt_l);
	*out_r = (int32_t) (acc_r / (int64_t) cnt_r);
}

/* Locate chunks and fill fmt/data metadata.
 * Accepts PCM (0x0001) and IEEE float (0x0003).
 */
static bool parse_wav_header(const uint8_t *wav, size_t wav_bytes, wav_fmt_t *fmt, const uint8_t **data_ptr, uint32_t *data_bytes) {
	if (wav_bytes < 12) {
		return false;
	}
	if (memcmp(wav + 0, "RIFF", 4) != 0 || memcmp(wav + 8, "WAVE", 4) != 0) {
		return false;
	}

	size_t off = 12;
	bool got_fmt = false;
	bool got_data = false;

	while (off + 8 <= wav_bytes) {
		const uint8_t *chdr = wav + off;
		const uint32_t *szp = (const uint32_t *) (chdr + 4);
		uint32_t ck_size = *szp;
		const uint8_t *cdat = chdr + 8;

		if (off + 8 + ck_size > wav_bytes) {
			return false;
		}

		if (memcmp(chdr, "fmt ", 4) == 0) {
			if (ck_size < 16) {
				return false;
			}
			fmt->format_tag = *(const uint16_t *) (cdat + 0);
			fmt->channels = *(const uint16_t *) (cdat + 2);
			fmt->samplerate = *(const uint32_t *) (cdat + 4);
			fmt->byterate = *(const uint32_t *) (cdat + 8);
			fmt->block_align = *(const uint16_t *) (cdat + 12);
			fmt->bits_per_sample = *(const uint16_t *) (cdat + 14);
			got_fmt = true;
		} else if (memcmp(chdr, "data", 4) == 0) {
			*data_ptr = cdat;
			*data_bytes = ck_size;
			got_data = true;
		}

		size_t adv = 8u + (size_t) ck_size;
		if ((adv & 1u) != 0u) {
			adv++;
		}
		off += adv;

		if (got_fmt && got_data) {
			break;
		}
	}

	if (!got_fmt || !got_data) {
		return false;
	}

	if (fmt->format_tag == 0x0001) {
		if (fmt->bits_per_sample != 8 && fmt->bits_per_sample != 16 && fmt->bits_per_sample != 24 && fmt->bits_per_sample != 32) {
			return false;
		}
	} else if (fmt->format_tag == 0x0003) {
		if (fmt->bits_per_sample != 32 && fmt->bits_per_sample != 64) {
			return false;
		}
	} else {
		return false;
	}

	if (fmt->channels < 1 || fmt->samplerate == 0 || fmt->block_align == 0) {
		return false;
	}
	return true;
}

size_t wav_size_to_samples(size_t length) {
	return length / sizeof(int16_t) / 2;
}

size_t wav_samples_to_size(size_t samples) {
	return samples * sizeof(int16_t) * 2;
}

bool audio_wav_load(const char *filename, void **out_ptr, size_t *out_bytes) {
	if (!out_ptr || !out_bytes || !filename) {
		return false;
	}
	fs_directory_entry_t *entry = fs_get_file_info(filename);
	if (!entry || (entry->flags & FS_DIRECTORY) != 0) {
		return false;
	}
	uint8_t *data = kmalloc(entry->size);
	if (!data) {
		return false;
	}
	if (!fs_read_file(entry, 0, entry->size, data)) {
		kfree(data);
		return false;
	}
	bool result = wav_from_memory(data, entry->size, out_ptr, out_bytes);
	kfree(data);
	return result;
}

bool wav_from_memory(const void *wav, size_t wav_bytes, void **out_ptr, size_t *out_bytes) {
	const uint8_t *p = (const uint8_t *) wav;
	wav_fmt_t fmt;
	const uint8_t *src_data = NULL;
	uint32_t src_data_bytes = 0;

	if (out_ptr == NULL || out_bytes == NULL) {
		return false;
	}
	*out_ptr = NULL;
	*out_bytes = 0;

	if (!parse_wav_header(p, wav_bytes, &fmt, &src_data, &src_data_bytes)) {
		return false;
	}

	const bool is_float = (fmt.format_tag == 0x0003u);

	const uint32_t src_channels = fmt.channels;
	const uint16_t src_bits = fmt.bits_per_sample;
	const uint32_t src_rate = fmt.samplerate;
	const uint16_t bytes_per_sample = (uint16_t) ((src_bits + 7u) / 8u);
	const uint32_t src_block = (uint32_t) src_channels * (uint32_t) bytes_per_sample;

	if (src_block == 0u) {
		return false;
	}
	if (src_data_bytes < src_block) {
		return false;
	}

	const uint64_t src_frames_u64 = (uint64_t) src_data_bytes / (uint64_t) src_block;
	if (src_frames_u64 == 0u) {
		return false;
	}
	if (src_frames_u64 > 0x7FFFFFFFu) {
		return false;
	}
	const uint32_t src_frames = src_frames_u64;

	const uint32_t dst_rate = 44100u;
	uint64_t dst_frames_u64 = ((uint64_t) src_frames * (uint64_t) dst_rate + (uint64_t) src_rate - 1u) / (uint64_t) src_rate;
	if (dst_frames_u64 == 0u) {
		return false;
	}
	if (dst_frames_u64 > (UINT64_C(1) << 31)) {
		return false;
	}
	const uint32_t dst_frames = (uint32_t) dst_frames_u64;

	const size_t dst_total_bytes = (size_t) dst_frames * 2u * sizeof(int16_t);
	int16_t *dst = (int16_t *) kmalloc(dst_total_bytes);
	if (dst == NULL) {
		return false;
	}

	uint64_t pos_q32 = 0;
	const uint64_t step_q32 = (((uint64_t) src_rate) << 32) / (uint64_t) dst_rate;

	const uint8_t *frame0;
	const uint8_t *frame1;
	uint32_t idx0;
	uint32_t idx1;
	uint32_t i;

	for (i = 0; i < dst_frames; i++) {
		idx0 = (uint32_t) (pos_q32 >> 32);
		uint32_t frac = (uint32_t) (pos_q32 & 0xFFFFFFFFu);

		if (idx0 >= src_frames) {
			idx0 = src_frames - 1u;
		}
		idx1 = idx0 + 1u;
		if (idx1 >= src_frames) {
			idx1 = src_frames - 1u;
		}

		frame0 = src_data + (size_t) idx0 * (size_t) src_block;
		frame1 = src_data + (size_t) idx1 * (size_t) src_block;

		int32_t l0, r0, l1, r1;
		fetch_frame_lr_s16(frame0, fmt.channels, fmt.bits_per_sample, is_float, &l0, &r0);
		fetch_frame_lr_s16(frame1, fmt.channels, fmt.bits_per_sample, is_float, &l1, &r1);

		int32_t dl = l1 - l0;
		int32_t dr = r1 - r0;

		int32_t lo = l0 + (int32_t) ((((int64_t) dl) * (int64_t) frac) >> 32);
		int32_t ro = r0 + (int32_t) ((((int64_t) dr) * (int64_t) frac) >> 32);

		lo = CLAMP(lo, -32768, 32767);
		ro = CLAMP(ro, -32768, 32767);

		dst[i * 2u + 0u] = (int16_t) lo;
		dst[i * 2u + 1u] = (int16_t) ro;

		pos_q32 += step_q32;
		if ((pos_q32 >> 32) > (uint64_t) (src_frames - 1u)) {
			pos_q32 = ((uint64_t) (src_frames - 1u) << 32);
		}
	}

	*out_ptr = (void *) dst;
	*out_bytes = dst_total_bytes;
	return true;
}
