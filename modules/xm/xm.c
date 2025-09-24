#include <kernel.h>
#include "xm.h"

static audio_file_loader_t *g_xm_loader = NULL;

static inline uint16_t rd16(const uint8_t *p) {
	return (uint16_t) (p[0] | (p[1] << 8));
}

static inline uint32_t rd32(const uint8_t *p) {
	return (uint32_t) (p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static int16_t *xm_delta_decode(const uint8_t *src, uint32_t len_bytes, int sixteen_bit) {
	if (sixteen_bit) {
		uint32_t count = len_bytes / 2;
		int16_t *dst = (int16_t *) kmalloc(count * sizeof(int16_t));
		if (!dst) {
			return NULL;
		}
		int32_t acc = 0;
		for (uint32_t i = 0; i < count; i++) {
			int16_t d = (int16_t) (src[i * 2] | (src[i * 2 + 1] << 8));
			acc += d;
			dst[i] = (int16_t) acc;
		}
		return dst;
	} else {
		uint32_t count = len_bytes;
		int16_t *dst = (int16_t *) kmalloc(count * sizeof(int16_t));
		if (!dst) {
			return NULL;
		}
		int32_t acc = 0;
		for (uint32_t i = 0; i < count; i++) {
			int8_t d = (int8_t) src[i];
			acc += d;
			dst[i] = (int16_t) (acc << 8);
		}
		return dst;
	}
}

static int clamp_int(int v, int lo, int hi) {
	if (v < lo) {
		return lo;
	}
	if (v > hi) {
		return hi;
	}
	return v;
}

/* FT2 linear frequency, exact keying:
   - 64 "key units" per semitone (12 semis per octave)
   - key64 = (note-1)*64 + rel_note*64 + fine/2   (fine is -128..127 -> -64..+63)
   - C-4 (note 49) maps to key64 = 3072 and must produce 8363 Hz
   - frequency = 8363 * 2^((key64 - 3072) / 768)
   - step_q16 = frequency / samplerate in Q16.16
*/
/* FT2 linear frequency, spec-exact.
   - RealNote = (note_1_96 - 1) + relative_note    (0..118)
   - Period   = 7680 - RealNote*64 - fine_tune_128/2
   - Freq(Hz) = 8363 * 2^((4608 - Period) / 768)
   - step(Q16)= (Freq * 65536) / samplerate
*/
static int compute_step_linear_q16(int samplerate, int note_1_96, int relative_note, int fine_tune_128)
{
	int rn = (note_1_96 - 1) + relative_note;         /* RealNote */
	if (rn < 0)  rn = 0;
	if (rn > 118) rn = 118;

	int period = 7680 - rn * 64 - (fine_tune_128 / 2);

	double expo = (4608.0 - (double)period) / 768.0;
	double freq = 8363.0 * pow(2.0, expo);

	int step = (int)((freq * 65536.0) / (double)samplerate);
	if (step < 1) step = 1;
	return step;
}

/* Amiga-ish path: compute step (Q16) using PAL clock approximation */
static int compute_step_amiga_q16(uint32_t sample_rate_q16, int note_1_96, int rel_note, int fine_tune_128) {
	int n = note_1_96;
	if (n < 1) {
		n = 1;
	}
	if (n > 96) {
		n = 96;
	}

	int semis = (n - 49) + rel_note;
	double frac = (double) semis + ((double) fine_tune_128 / 128.0);

	/* Period halves every 12 semitones; base 428 at C-4 */
	double period = 428.0 * pow(2.0, -frac / 12.0);
	if (period < 1.0) {
		period = 1.0;
	}
	uint32_t p = (uint32_t) period;
	if (p == 0) {
		p = 1;
	}

	return (int) (sample_rate_q16 / p);
}

/* Recompute sample_inc (Q16.16) from effective pitch.
   - base_note_1_96: 1..96
   - semi_offset128: extra pitch in 1/128 semitone units (can be negative), e.g.
       * arpeggio: +n*128
       * vibrato:  +/- depth_scaled in 1/128 semitone
*/
static inline void set_step_from_note(xm_state_t *st, xm_channel_t *c,
				      int base_note_1_96, int semi_offset128)
{
	if (!c->smp || !c->smp->pcm) {
		c->sample_inc = 0;
		return;
	}

	/* Fold fractional semitone offset into fine tune */
	int note = base_note_1_96;
	int fine128 = c->smp->fine_tune + semi_offset128;

	/* Normalize: keep fine within [-64*128, +64*128] around note so exponent stays sane */
	while (fine128 >= 64 * 128) { note += 64; fine128 -= 64 * 128; }
	while (fine128 <= -64 * 128){ note -= 64; fine128 += 64 * 128; }

	if (st->flags_linear_freq) {
		/* FT2 linear: convert the extra 1/128 semitone offset to linear fine_tune (1/2 key64 unit):
		   1 semitone = 64 key64 units; FineTune term in Period is (fine/2), where 'fine' is in 1/1 semitone = 128 units.
		   So we can pass the combined fine directly as 'fine_tune_128'. */
		c->sample_inc = compute_step_linear_q16((int)st->samplerate, note, c->smp->relative_note, fine128);
	} else {
		/* Amiga/period path (your existing function), unchanged */
		uint32_t sr_q16 = (uint32_t)(((uint64_t)3546895 << 16) / (uint64_t)st->samplerate);
		c->sample_inc = compute_step_amiga_q16(sr_q16, note, c->smp->relative_note, c->smp->fine_tune + (semi_offset128 / 128));
	}
}


/* Envelope evaluator (linear between points) */
static int env_value_linear(const xm_envelope_t *env, int tick, int is_vol) {
	if (!env->enabled || env->num_points == 0) {
		return is_vol ? 64 : 128; /* unity volume, centre pan */
	}

	/* Handle loop */
	int end_x = env->points[env->num_points - 1][0];
	if (env->loop_end > env->loop_start && env->loop_end < env->num_points) {
		int ls_x = env->points[env->loop_start][0];
		int le_x = env->points[env->loop_end][0];
		if (le_x > ls_x) {
			if (tick > le_x) {
				int span = le_x - ls_x;
				int t2 = (span > 0) ? ((tick - ls_x) % (span)) + ls_x : ls_x;
				tick = t2;
			}
		}
	} else {
		if (tick > end_x) {
			tick = end_x;
		}
	}

	/* Find segment */
	int i = 0;
	for (i = 0; i < env->num_points - 1; i++) {
		int x0 = env->points[i][0];
		int x1 = env->points[i + 1][0];
		if (tick >= x0 && tick <= x1) {
			int y0 = env->points[i][1];
			int y1 = env->points[i + 1][1];
			int dx = x1 - x0;
			if (dx <= 0) {
				return is_vol ? clamp_int(y0, 0, 64) : clamp_int(y0, 0, 255);
			}
			int dy = y1 - y0;
			int num = (tick - x0) * dy;
			int y = y0 + num / dx;
			return is_vol ? clamp_int(y, 0, 64) : clamp_int(y, 0, 255);
		}
	}
	/* After last point */
	int y_end = env->points[env->num_points - 1][1];
	return is_vol ? clamp_int(y_end, 0, 64) : clamp_int(y_end, 0, 255);
}

static int decode_patterns(const uint8_t *base, size_t len, int pat_count, int channels, xm_pattern_t *out_patterns, const uint8_t **cursor) {
	const uint8_t *p = *cursor;

	for (int pi = 0; pi < pat_count; pi++) {
		if ((size_t)(p - base) + 9 > len) {
			dprintf("xm: pattern[%d] header OOB\n", pi);
			return 0;
		}

		uint32_t header_len = rd32(p); p += 4;
		if ((size_t)(p - base) + header_len - 4 > len) {
			dprintf("xm: pattern[%d] header truncated\n", pi);
			return 0;
		}

		uint16_t rows        = rd16(p + 1);
		uint16_t packed_len  = rd16(p + 3);
		p += header_len - 4;

		if (rows == 0) {
			rows = 64;
		}
		if ((size_t)(p - base) + packed_len > len) {
			dprintf("xm: pattern[%d] data OOB\n", pi);
			return 0;
		}

		xm_pattern_t *pat = &out_patterns[pi];
		pat->rows = rows;
		size_t cell_count = (size_t)rows * (size_t)channels;
		pat->cells = (xm_cell_t *)kmalloc(cell_count * sizeof(xm_cell_t));
		if (!pat->cells) {
			dprintf("xm: pattern[%d] alloc fail\n", pi);
			return 0;
		}
		memset(pat->cells, 0, cell_count * sizeof(xm_cell_t));

		const uint8_t *d = p;
		const uint8_t *dend = p + packed_len;

		for (uint16_t r = 0; r < rows; r++) {
			for (int ch = 0; ch < channels; ch++) {
				if (d >= dend) {
					dprintf("xm: pattern[%d] truncated at row %u ch %d\n", pi, r, ch);
					return 0;
				}

				uint8_t a = *d++;
				uint8_t note = 0, inst = 0, vol = 0, eff = 0, par = 0;

				if (a & 0x80) {
					uint8_t flags = a;
					if (flags & 0x01 && d < dend) note = *d++;
					if (flags & 0x02 && d < dend) inst = *d++;
					if (flags & 0x04 && d < dend) vol  = *d++;
					if (flags & 0x08 && d < dend) eff  = *d++;
					if (flags & 0x10 && d < dend) par  = *d++;
				} else {
					note = a;
					if (d + 4 > dend) {
						dprintf("xm: pattern[%d] cell truncated at row %u ch %d\n", pi, r, ch);
						return 0;
					}
					inst = *d++;
					vol  = *d++;
					eff  = *d++;
					par  = *d++;
				}

				xm_cell_t *cell = &pat->cells[r * channels + ch];
				cell->note       = note;
				cell->instrument = inst;
				cell->volcol     = vol;
				cell->effect     = eff;
				cell->param      = par;
			}
		}

		p += packed_len;
	}

	*cursor = p;
	return 1;
}


static int decode_instruments(const uint8_t *base, size_t len, int instr_count,
			      xm_instrument_t *out_instr, const uint8_t **cursor) {
	const uint8_t *p = *cursor;

	for (int ii = 0; ii < instr_count; ii++) {
		/* Need 4 bytes for instrument size */
		if ((size_t)(p - base) + 4 > len) {
			dprintf("xm: instrument[%d] header size OOB cur_off=%lu need=4 file_len=%lu\n",
				ii, (size_t)(p - base), len);
			return 0;
		}

		/* Instrument header starts at inst0 = p, size includes this DWORD */
		const uint8_t *inst0 = p;
		uint32_t hdr_size = rd32(inst0);

		/* Entire header (hdr_size bytes from inst0) must fit */
		size_t hdr_end = (size_t)(inst0 - base) + hdr_size;
		if (hdr_end > len || hdr_size < 4) {
			dprintf("xm: instrument[%d] header truncated hdr_size=%u cur_off=%lu need_end=%lu file_len=%lu\n",
				ii, hdr_size, (size_t)(p - base), hdr_end, len);
			return 0;
		}

		/* Body begins after the size field */
		const uint8_t *hdr = inst0 + 4;

		/* num_samples is at +27 from instrument start => +23 from hdr */
		int num_samples = 0;
		if (hdr_size >= 29) {
			num_samples = rd16(hdr + 23);
		}

		/* sample_header_size is at +29 from instrument start => +25 from hdr.
		   BUT many trackers write nonsense here; clamp to 40 when out-of-range. */
		uint32_t sample_hdr_size = 40;
		if (num_samples > 0) {
			if (hdr_size >= 33) {
				sample_hdr_size = rd32(hdr + 25);
			}
			if (sample_hdr_size < 40 || sample_hdr_size > 0x400) {
				/* FT2 ignores this value; most sane files use 40. */
				sample_hdr_size = 40;
			}
		}

		/* Prepare output instrument */
		xm_instrument_t *ins = &out_instr[ii];
		memset(ins, 0, sizeof(*ins));
		ins->num_samples = num_samples;
		for (int k = 0; k < 96; k++) ins->keymap[k] = -1;

		/* Keymap + envelopes only present when header is long enough */
		if (num_samples > 0 && hdr_size >= 263) {
			/* keymap at +33 from inst start => +29 from hdr */
			for (int k = 0; k < 96; k++) {
				int v = hdr[29 + k];
				ins->keymap[k] = (v > 0) ? (v - 1) : -1;
			}

			xm_envelope_t *ve = &ins->vol_env;
			xm_envelope_t *pe = &ins->pan_env;

			/* volume envelope points at +129 => +125 from hdr */
			for (int i = 0; i < XM_MAX_ENV_POINTS; i++) {
				ve->points[i][0] = rd16(hdr + 125 + i * 4);
				ve->points[i][1] = clamp_int(rd16(hdr + 125 + i * 4 + 2), 0, 64);
			}
			/* pan envelope points at +177 => +173 from hdr */
			for (int i = 0; i < XM_MAX_ENV_POINTS; i++) {
				pe->points[i][0] = rd16(hdr + 173 + i * 4);
				pe->points[i][1] = clamp_int(rd16(hdr + 173 + i * 4 + 2), 0, 255);
			}

			ve->num_points = hdr[221];
			pe->num_points = hdr[222];
			ve->sustain    = hdr[223];
			ve->loop_start = hdr[224];
			ve->loop_end   = hdr[225];
			pe->sustain    = hdr[226];
			pe->loop_start = hdr[227];
			pe->loop_end   = hdr[228];
			ve->enabled    = (hdr[229] & 0x01) ? 1 : 0;
			pe->enabled    = (hdr[230] & 0x01) ? 1 : 0;
			ins->fadeout   = rd16(hdr + 235);
		}

		/* Jump to end of instrument header: this is where sample headers start */
		p = inst0 + hdr_size;

		if (num_samples <= 0) {
			continue; /* no sample headers nor sample data */
		}

		/* Sample headers block: num_samples * sample_hdr_size bytes */
		size_t shead_off = (size_t)(p - base);
		size_t shead_end = shead_off + (size_t)sample_hdr_size * (size_t)num_samples;
		if (shead_end > len) {
			dprintf("xm: instrument[%d] sample headers OOB cur_off=%lu hdr_size=%u num_samples=%d need_end=%lu file_len=%lu\n",
				ii, shead_off, sample_hdr_size, num_samples, shead_end, len);
			return 0;
		}

		const uint8_t *sheaders = p;
		p += (size_t)sample_hdr_size * (size_t)num_samples; /* advance to first sample data */

		/* Allocate sample array */
		xm_sample_t *sarr = (xm_sample_t *) kmalloc(sizeof(xm_sample_t) * (size_t)num_samples);
		if (!sarr) {
			dprintf("xm: instrument[%d] sample array OOM num_samples=%d\n", ii, num_samples);
			return 0;
		}
		memset(sarr, 0, sizeof(xm_sample_t) * (size_t)num_samples);
		ins->samples = sarr;

		/* Pass 1: read sample metadata */
		for (int si = 0; si < num_samples; si++) {
			const uint8_t *sh = sheaders + (size_t)si * sample_hdr_size;

			uint32_t slen       = rd32(sh + 0);
			uint32_t loop_start = rd32(sh + 4);
			uint32_t loop_len   = rd32(sh + 8);
			uint8_t  vol        = sh[12];
			int8_t   fine       = (int8_t)sh[13];
			uint8_t  type       = sh[14];
			uint8_t  pan        = sh[15];
			int8_t   rel        = (int8_t)sh[16];

			int sixteen_bit = (type & 0x10) ? 1 : 0;
			int loop_type = 0;
			if (type & 0x03) loop_type = (type & 0x02) ? 2 : 1;

			xm_sample_t *smp = &sarr[si];
			smp->length        = sixteen_bit ? (slen / 2) : slen;
			smp->loop_start    = sixteen_bit ? (loop_start / 2) : loop_start;
			smp->loop_len      = sixteen_bit ? (loop_len / 2) : loop_len;
			smp->loop_type     = loop_type;
			smp->relative_note = rel;
			smp->fine_tune     = fine;
			smp->default_volume= clamp_int(vol, 0, 64);
			smp->default_pan   = pan;
		}

		/* Pass 2: read & delta-decode sample payloads (immediately after all headers) */
		for (int si = 0; si < num_samples; si++) {
			const uint8_t *sh = sheaders + (size_t)si * sample_hdr_size;
			uint32_t slen = rd32(sh + 0);
			uint8_t type  = sh[14];
			int sixteen_bit = (type & 0x10) ? 1 : 0;

			size_t cur_off = (size_t)(p - base);
			size_t need_end = cur_off + slen;
			if (need_end > len) {
				dprintf("xm: instrument[%d] sample[%d] payload OOB slen=%u cur_off=%lu need_end=%lu file_len=%lu\n",
					ii, si, slen, cur_off, need_end, len);
				return 0;
			}

			if (slen == 0) {
				sarr[si].pcm = NULL;
			} else {
				int16_t *pcm = xm_delta_decode(p, slen, sixteen_bit);
				if (!pcm) {
					dprintf("xm: instrument[%d] sample[%d] delta decode OOM slen=%u\n", ii, si, slen);
					return 0;
				}
				sarr[si].pcm = pcm;
			}
			p += slen;
		}
	}

	*cursor = p;
	return 1;
}

static int parse_xm_and_build(const char *name, const uint8_t *bytes, size_t len, xm_state_t *st) {
	memset(st, 0, sizeof(*st));
	st->samplerate = XM_TARGET_RATE;
	st->global_vol = 64;
	st->rng = 0x13579B;

	if (len < 60 || memcmp(bytes, XM_MAGIC_TEXT, XM_MAGIC_LEN) != 0) {
		return 0;
	}

	const uint8_t *p = bytes + XM_MAGIC_LEN;
	/* Title (20), 0x1A, tracker name (20), version (2), header size (4) */
	p += 20; /* title */
	if ((size_t) (p - bytes) + 1 > len) {
		return 0;
	}
	p += 1;  /* 0x1A */
	p += 20; /* tracker name */
	if ((size_t) (p - bytes) + 2 + 4 > len) {
		return 0;
	}
	uint16_t ver = rd16(p); p += 2;
	(void) ver;
	uint32_t hdr_size = rd32(p); p += 4;

	if ((size_t) (p - bytes) + hdr_size - 4 > len) {
		return 0;
	}

	/* Song header fields */
	uint16_t song_len   = rd16(p + 0);
	uint16_t restart    = rd16(p + 2); (void) restart;
	uint16_t channels   = rd16(p + 4);
	uint16_t pat_count  = rd16(p + 6);
	uint16_t ins_count  = rd16(p + 8);
	uint16_t flags      = rd16(p + 10);
	uint16_t tempo      = rd16(p + 12);
	uint16_t bpm        = rd16(p + 14);

	if (channels == 0 || channels > XM_MAX_CHANNELS) {
		dprintf("xm: invalid channel count %u\n", channels);
		return 0;
	}
	if (song_len == 0 || song_len > XM_MAX_ORDERS) {
		dprintf("xm: invalid song length %u\n", song_len);
		return 0;
	}
	if (pat_count > XM_MAX_PATTERNS || ins_count > XM_MAX_INSTRUMENTS) {
		dprintf("xm: patterns/instruments too many\n");
		return 0;
	}

	st->channels = (int) channels;
	st->orders = (int) song_len;
	st->patterns_count = (int) pat_count;
	st->instruments_count = (int) ins_count;
	st->flags_linear_freq = (flags & 0x01) ? 1 : 0;
	dprintf("xm: flags linear freq: %d\n", st->flags_linear_freq);
	st->initial_speed = (tempo == 0) ? 6 : (int) tempo;
	st->initial_tempo = (bpm < 32) ? 125 : (int) bpm;

	st->speed = st->initial_speed;
	st->tempo = st->initial_tempo;
	st->audio_speed = (st->samplerate * 5) / (2 * st->tempo);
	st->audio_tick = st->audio_speed;
	st->audio_tick = st->audio_speed;

	st->order = 0;
	st->row = 0;
	st->tick = 0;

	/* Order table (256 bytes max, but only song_len used) */
	const uint8_t *order_table = p + 16;
	if ((size_t) (order_table - bytes) + 256 > len) {
		return 0;
	}
	st->order_table = order_table;

	/* Move past song header */
	p = bytes + XM_MAGIC_LEN      /* 17 */
	    + 20                /* title */
	    + 1                 /* 0x1A */
	    + 20                /* tracker */
	    + 2                 /* version */
	    + 4                 /* header size field */
	    + (hdr_size - 4);   /* remaining song header body */
	/* Patterns */
	st->patterns = (xm_pattern_t *) kmalloc(sizeof(xm_pattern_t) * (size_t) st->patterns_count);
	if (!st->patterns) {
		dprintf("xm: patterns alloc OOM\n");
		return 0;
	}
	memset(st->patterns, 0, sizeof(xm_pattern_t) * (size_t) st->patterns_count);

	if (!decode_patterns(bytes, len, st->patterns_count, st->channels, st->patterns, &p)) {
		return 0;
	}

	/* Instruments + samples */
	st->instruments = (xm_instrument_t *) kmalloc(sizeof(xm_instrument_t) * (size_t) st->instruments_count);
	if (!st->instruments) {
		dprintf("xm: instruments alloc OOM\n");
		return 0;
	}
	memset(st->instruments, 0, sizeof(xm_instrument_t) * (size_t) st->instruments_count);

	if (!decode_instruments(bytes, len, st->instruments_count, st->instruments, &p)) {
		return 0;
	}

	/* Init channels */
	for (int i = 0; i < st->channels; i++) {
		xm_channel_t *c = &st->ch[i];
		memset(c, 0, sizeof(*c));
		c->inst_idx = -1;
		c->smp_idx = -1;
		c->key_on = 0;
		c->vol = 64;
		c->pan = 128; /* centre */
		c->loop_dir = 1;
		c->retrig_countdown = 0;
		c->global_vol_applied = -1;
	}

	/* Log title for parity with MOD loader */
	char title[21];
	memset(title, 0, sizeof(title));
	memcpy(title, bytes + XM_MAGIC_LEN, 20);
	for (int i = 19; i >= 0; i--) {
		if (title[i] == ' ' || title[i] == 0) {
			title[i] = 0;
		} else {
			break;
		}
	}
	dprintf("Music module '%s': title: '%s' channels: %d orders: %d\n", name, title, st->channels, st->orders);

	return 1;
}

static void update_frequency_from_note(xm_state_t *st, xm_channel_t *c, int note_1_96) {
	if (!c->smp) {
		c->sample_inc = 0;
		return;
	}
	if (st->flags_linear_freq) {
		c->sample_inc = compute_step_linear_q16((int) st->samplerate, note_1_96, c->smp->relative_note, c->smp->fine_tune);
	} else {
		c->sample_inc = compute_step_amiga_q16((uint32_t) (((uint64_t) 3546895 << 16) / (uint64_t) st->samplerate), note_1_96, c->smp->relative_note, c->smp->fine_tune);
	}
}

static void apply_note_on(xm_state_t *st, xm_channel_t *c, int note_1_96, int inst_1_128) {
	if (inst_1_128 > 0 && inst_1_128 <= st->instruments_count) {
		c->inst_idx = inst_1_128 - 1;
	}

	c->curr_note = note_1_96;
	c->smp = NULL;
	c->smp_idx = -1;

	if (c->inst_idx >= 0 && c->inst_idx < st->instruments_count) {
		xm_instrument_t *ins = &st->instruments[c->inst_idx];
		if (ins->num_samples > 0) {
			int map_idx = note_1_96 - 1;
			if (map_idx < 0) {
				map_idx = 0;
			}
			if (map_idx > 95) {
				map_idx = 95;
			}
			int si = ins->keymap[map_idx];
			if (si < 0 || si >= ins->num_samples) {
				si = 0; /* fallback */
			}

			c->smp_idx = si;
			c->smp = &ins->samples[si];

			/* Always set volume/pan to sample defaults on note-on */
			c->vol = clamp_int(c->smp->default_volume, 0, 64);
			c->pan = clamp_int(c->smp->default_pan, 0, 255);
		}
	}

	c->key_on = 1;
	c->released = 0;
	c->fade = 0;

	c->sample_pos = 0;
	c->sample_frac = 0;
	c->loop_dir = 1;

	if (c->smp && c->smp->pcm) {
		int step_q16;
		if (st->flags_linear_freq) {
			step_q16 = compute_step_linear_q16((int)st->samplerate,
							   note_1_96,
							   c->smp->relative_note,
							   c->smp->fine_tune);
		} else {
			step_q16 = compute_step_amiga_q16((uint32_t)(((uint64_t)3546895 << 16) /
								     (uint64_t)st->samplerate),
							  note_1_96,
							  c->smp->relative_note,
							  c->smp->fine_tune);
		}
		c->sample_inc = step_q16;
	} else {
		c->sample_inc = 0;
	}

	/* reset envelopes */
	c->vol_env_tick = 0;
	c->pan_env_tick = 0;
}

static void apply_volume_column(xm_state_t *st, xm_channel_t *c, uint8_t volcol, int tick_zero) {
	if (volcol == 0) {
		return;
	}
	uint8_t hi = (volcol & 0xF0);
	uint8_t lo = (volcol & 0x0F);

	if (volcol >= 0x10 && volcol <= 0x50) {
		int v = (int) volcol - 0x10;
		if (v > 64) {
			v = 64;
		}
		c->vol = v;
		return;
	}

	switch (hi) {
		case 0x60: { /* vol slide down */
			if (!tick_zero) {
				c->vol -= (int) lo;
				if (c->vol < 0) {
					c->vol = 0;
				}
			}
			break;
		}
		case 0x70: { /* vol slide up */
			if (!tick_zero) {
				c->vol += (int) lo;
				if (c->vol > 64) {
					c->vol = 64;
				}
			}
			break;
		}
		case 0x80: { /* fine vol down (tick 0) */
			if (tick_zero) {
				c->vol -= (int) lo;
				if (c->vol < 0) {
					c->vol = 0;
				}
			}
			break;
		}
		case 0x90: { /* fine vol up (tick 0) */
			if (tick_zero) {
				c->vol += (int) lo;
				if (c->vol > 64) {
					c->vol = 64;
				}
			}
			break;
		}
		case 0xA0: { /* vibrato speed shorthand */
			c->vib_speed = (int) lo;
			break;
		}
		case 0xB0: { /* vibrato depth shorthand */
			c->vib_depth = (int) lo;
			break;
		}
		case 0xC0: { /* set pan (0..15 => 0..240) */
			c->pan = clamp_int(lo * 16, 0, 255);
			break;
		}
		case 0xD0: { /* pan slide left */
			if (!tick_zero) {
				c->pan -= (int) lo * 4;
				if (c->pan < 0) {
					c->pan = 0;
				}
			}
			break;
		}
		case 0xE0: { /* pan slide right */
			if (!tick_zero) {
				c->pan += (int) lo * 4;
				if (c->pan > 255) {
					c->pan = 255;
				}
			}
			break;
		}
		case 0xF0: { /* portamento to note shorthand (speed=lo) */
			if (lo != 0) {
				c->porta_speed = (int) lo;
			}
			/* tick work handled in 3xx handler */
			break;
		}
		default:
			break;
	}
}

static void pattern_control_after_row_advance(xm_state_t *st) {
	st->row++;
	if (st->row >= st->patterns[st->order_table[st->order]].rows) {
		st->row = 0;
		int prev_order = st->order;
		st->order++;
		if (st->order >= st->orders) {
			st->ended = 1;
			st->order = 0;
		}
		if (st->order <= prev_order) {
			st->ended = 1;
		}
	}
}

/* Tick 0: read row cells and apply immediate ops; non-zero ticks: continuous effects */
static void engine_tick(xm_state_t *st) {
	if (st->tick == 0) {
		/* reset per-row loop triggers */
		int skip_order_req = -1;
		int skip_dest_row = 0;

		xm_pattern_t *pat = &st->patterns[st->order_table[st->order]];

		for (int ch = 0; ch < st->channels; ch++) {
			xm_channel_t *c = &st->ch[ch];
			xm_cell_t *cell = &pat->cells[st->row * st->channels + ch];

			int note = cell->note;
			int inst = cell->instrument;
			int eff = cell->effect;
			int ev  = cell->param;
			uint8_t volcol = cell->volcol;

			/* Instrument change without note sets defaults */
			if (inst) {
				if (inst <= st->instruments_count) {
					c->inst_idx = inst - 1;
					/* Set sample defaults if present */
					xm_instrument_t *ins = &st->instruments[c->inst_idx];
					if (ins->num_samples > 0) {
						c->smp_idx = 0;
						c->smp = &ins->samples[0];
						c->vol = c->smp->default_volume;
						c->pan = c->smp->default_pan;
					}
				}
			}

			if (note == 97) {
				/* Note Off: release envelopes and start fadeout */
				c->key_on = 0;
				c->released = 1;
			} else if (note > 0 && note <= 96) {
				if (eff == 0x3 || eff == 0x5) {
					/* Tone portamento: do not reset sample; set target only */
					c->target_note = note;
					if (c->porta_speed == 0 && ev != 0) {
						c->porta_speed = ev;
					}
				} else {
					apply_note_on(st, c, note, inst);
				}
			}

			/* Volume column (tick 0 part) */
			apply_volume_column(st, c, volcol, 1);

			/* Tick-0 for certain effects */
			switch (eff) {
				case 0x3: { /* tone portamento: target already set */
					if (ev != 0) {
						c->porta_speed = ev;
					}
					break;
				}
				case 0x4: { /* vibrato set speed/depth nibbles on tick 0 */
					if (ev & 0xF0) {
						c->vib_speed = (ev >> 4) & 0x0F;
					}
					if (ev & 0x0F) {
						c->vib_depth = ev & 0x0F;
					}
					break;
				}
				case 0x6: { /* vibrato + vol slide: init vib speed/depth from previous if zero */
					/* nothing special here on tick 0 besides vib params already persisted */
					break;
				}
				case 0x7: { /* tremolo set speed/depth nibbles */
					if (ev & 0xF0) {
						c->trem_speed = (ev >> 4) & 0x0F;
					}
					if (ev & 0x0F) {
						c->trem_depth = ev & 0x0F;
					}
					break;
				}
				case 0x8: { /* set panning */
					c->pan = ev;
					break;
				}
				case 0x9: { /* sample offset: units of 256 bytes (8-bit) or 128 words (16-bit) */
					int offs_samples = 0;
					if (c->smp) {
						if (c->smp->pcm) {
							if (ev != 0) {
								if (c->smp->loop_len > 0 && c->smp->loop_start + c->smp->loop_len <= c->smp->length) {
									/* Offset relative to start */
								}
								c->sample_offset_mem = ev;
							}
							/* Our PCM is S16; original payload units are bytes/words.
							   Use 128 samples as unit to approximate both correctly. */
							offs_samples = (c->sample_offset_mem) * 128;
							if (offs_samples >= (int) c->smp->length) {
								offs_samples = (int) c->smp->length - 1;
							}
							c->sample_pos = offs_samples;
							c->sample_frac = 0;
						}
					}
					break;
				}
				case 0xB: { /* pattern jump */
					if (ev >= st->orders) {
						ev = 0;
					}
					skip_order_req = ev;
					skip_dest_row = 0;
					break;
				}
				case 0xC: { /* set volume */
					c->vol = clamp_int(ev, 0, 64);
					break;
				}
				case 0xD: { /* pattern break (BCD) */
					int row = ((ev >> 4) & 0x0F) * 10 + (ev & 0x0F);
					if (skip_order_req < 0) {
						if (st->order + 1 < st->orders) {
							skip_order_req = st->order + 1;
						} else {
							skip_order_req = 0;
						}
					}
					if (row > st->patterns[st->order_table[skip_order_req]].rows) {
						row = 0;
					}
					skip_dest_row = row;
					break;
				}
				case 0xE: { /* extended */
					int x = (ev >> 4) & 0x0F;
					int y = (ev & 0x0F);
					switch (x) {
						case 0x1: { /* fine porta up */
							c->curr_note += 0; /* no note change */
							if (c->sample_inc > 0) {
								/* emulate by small decrease in inc */
								/* handled in tick > 0 via 1xx; here do nothing */
							}
							break;
						}
						case 0x2: { /* fine porta down */
							break;
						}
						case 0x6: { /* pattern loop */
							if (y == 0) {
								st->pat_loop_row = st->row;
							} else {
								if (st->pat_loop_count == 0) {
									st->pat_loop_count = y + 1;
								}
								if (st->pat_loop_count > 1) {
									skip_order_req = st->order;
									skip_dest_row = st->pat_loop_row;
								}
								st->pat_loop_count--;
							}
							break;
						}
						case 0x9: { /* retrigger note every y ticks */
							if (y == 0) {
								y = 1;
							}
							c->retrig_countdown = y;
							break;
						}
						case 0xE: { /* pattern delay: extend current row by y+1 times */
							st->speed *= (y + 1);
							break;
						}
						default:
							break;
					}
					break;
				}
				case 0xF: { /* speed/tempo */
					if (ev != 0) {
						if (ev < 0x20) {
							/* set ticks per row */
							st->speed = ev;
						} else {
							/* set tempo (BPM) */
							st->tempo = ev;
							/* samples per tick = samplerate * 2.5 / BPM */
							st->audio_speed = (st->samplerate * 5) / (2 * st->tempo);
						}
					}
					break;
				}
				default:
					break;
			}

			/* Apply envelopes at tick 0 baseline */
			if (c->smp) {
				int ve = env_value_linear(&st->instruments[c->inst_idx].vol_env, c->vol_env_tick, 1);
				int pe = env_value_linear(&st->instruments[c->inst_idx].pan_env, c->pan_env_tick, 0);
				int v = (c->vol * ve) / 64;
				c->vol = clamp_int(v, 0, 64);
				/* pan envelope mixed 50/50 to taste */
				c->pan = clamp_int((c->pan + pe) / 2, 0, 255);
			}
		}

		/* Commit any flow control (Bxx/Dxx/E6x) after all channels examined */
		if (skip_order_req >= 0) {
			if (skip_order_req < st->order) {
				st->ended = 1;
			}
			st->order = skip_order_req;
			st->row = skip_dest_row;
		} else {
			/* advance row at end of tick block (handled below in tick wrap) */
		}
	} else {
		/* Non-zero ticks: continuous effects */
		for (int ch = 0; ch < st->channels; ch++) {
			xm_channel_t *c = &st->ch[ch];
			xm_pattern_t *pat = &st->patterns[st->order_table[st->order]];
			xm_cell_t *cell = &pat->cells[st->row * st->channels + ch];
			int eff = cell->effect;
			int ev  = cell->param;
			uint8_t volcol = cell->volcol;

			/* Volume column continuous part */
			apply_volume_column(st, c, volcol, 0);

			/* Retrigger countdown */
			if (c->retrig_countdown > 0) {
				c->retrig_countdown--;
				if (c->retrig_countdown == 0) {
					/* restart sample */
					if (c->smp && c->smp->pcm) {
						c->sample_pos = 0;
						c->sample_frac = 0;
						c->loop_dir = 1;
						c->retrig_countdown = (cell->param & 0x0F) ? (cell->param & 0x0F) : 1;
					}
				}
			}

			switch (eff) {
				case 0x0: { /* arpeggio: emulate with small pitch offsets by semitones */
					int a = (st->tick % 3);
					int semi = 0;
					if (a == 1) {
						semi = (cell->param >> 4) & 0x0F;
					} else if (a == 2) {
						semi = (cell->param & 0x0F);
					}
					if (c->smp) {
						int base = c->curr_note;
						int note = base + semi;
						update_frequency_from_note(st, c, note);
					}
					break;
				}
				case 0x1: { /* porta up */
					if (c->smp && c->sample_inc > 0) {
						/* raise pitch slightly */
						c->sample_inc += (ev << 4);
					}
					break;
				}
				case 0x2: { /* porta down */
					if (c->smp && c->sample_inc > 0) {
						int v = c->sample_inc - (ev << 4);
						if (v < 0) {
							v = 0;
						}
						c->sample_inc = v;
					}
					break;
				}
				case 0x3: { /* tone portamento */
					if (c->porta_speed == 0 && ev != 0) {
						c->porta_speed = ev;
					}
					if (c->smp) {
						int cur = c->curr_note;
						int tgt = c->target_note ? c->target_note : cur;
						if (cur < tgt) {
							cur += (c->porta_speed > 0) ? 1 : 0;
							if (cur > tgt) {
								cur = tgt;
							}
						} else if (cur > tgt) {
							cur -= (c->porta_speed > 0) ? 1 : 0;
							if (cur < tgt) {
								cur = tgt;
							}
						}
						c->curr_note = cur;
						update_frequency_from_note(st, c, c->curr_note);
					}
					break;
				}
				case 0x4: { /* vibrato */
					c->vib_phase = (c->vib_phase + c->vib_speed) & 63;
					/* sine table 0..63 -> -128..127 approximate */
					static const int8_t vib_sine[64] = {
						0,12,25,37,49,60,71,81,90,98,105,111,116,120,123,125,
						126,125,123,120,116,111,105,98,90,81,71,60,49,37,25,12,
						0,-12,-25,-37,-49,-60,-71,-81,-90,-98,-105,-111,-116,-120,-123,-125,
						-126,-125,-123,-120,-116,-111,-105,-98,-90,-81,-71,-60,-49,-37,-25,-12
					};
					int wave = vib_sine[c->vib_phase];
					int delta = (wave * c->vib_depth) >> 5; /* scale */
					if (c->smp) {
						int base = c->curr_note;
						update_frequency_from_note(st, c, base);
						c->sample_inc += delta;
						if (c->sample_inc < 0) {
							c->sample_inc = 0;
						}
					}
					break;
				}
				case 0x5: { /* tone porta + vol slide */
					/* vol slide */
					if (ev > 0x0F) {
						c->vol += (ev >> 4);
						if (c->vol > 64) {
							c->vol = 64;
						}
					} else {
						c->vol -= (ev & 0x0F);
						if (c->vol < 0) {
							c->vol = 0;
						}
					}
					/* fall-through: perform 3xx step */
					/* emulate one-semitone step per tick as above */
					if (c->smp && c->target_note) {
						int cur = c->curr_note;
						int tgt = c->target_note;
						if (cur < tgt) {
							cur += 1;
							if (cur > tgt) {
								cur = tgt;
							}
						} else if (cur > tgt) {
							cur -= 1;
							if (cur < tgt) {
								cur = tgt;
							}
						}
						c->curr_note = cur;
						update_frequency_from_note(st, c, c->curr_note);
					}
					break;
				}
				case 0x6: { /* vibrato + vol slide */
					if (ev > 0x0F) {
						c->vol += (ev >> 4);
						if (c->vol > 64) {
							c->vol = 64;
						}
					} else {
						c->vol -= (ev & 0x0F);
						if (c->vol < 0) {
							c->vol = 0;
						}
					}
					/* reuse 0x4 vibrato step */
					c->vib_phase = (c->vib_phase + c->vib_speed) & 63;
					break;
				}
				case 0x7: { /* tremolo */
					c->trem_phase = (c->trem_phase + c->trem_speed) & 63;
					/* implemented by modulating volume below */
					break;
				}
				case 0xA: { /* volume slide */
					if (ev > 0x0F) {
						c->vol += (ev >> 4);
						if (c->vol > 64) {
							c->vol = 64;
						}
					} else {
						c->vol -= (ev & 0x0F);
						if (c->vol < 0) {
							c->vol = 0;
						}
					}
					break;
				}
				default:
					break;
			}
		}
	}

	/* Envelope advancement per tick */
	for (int ch = 0; ch < st->channels; ch++) {
		xm_channel_t *c = &st->ch[ch];
		if (c->inst_idx >= 0) {
			xm_instrument_t *ins = &st->instruments[c->inst_idx];
			/* Sustain handling: if key_on and at sustain point x, do not advance beyond that point's x */
			if (ins->vol_env.enabled) {
				int sus_x = 0;
				if (ins->vol_env.sustain < ins->vol_env.num_points) {
					sus_x = ins->vol_env.points[ins->vol_env.sustain][0];
				}
				if (c->key_on) {
					if (c->vol_env_tick < sus_x) {
						c->vol_env_tick++;
					} else {
						/* hold */
					}
				} else {
					c->vol_env_tick++;
					if (c->released && ins->fadeout > 0) {
						c->fade += ins->fadeout;
						int fade_steps = c->fade >> 10;
						if (fade_steps > 0) {
							c->vol -= fade_steps;
							if (c->vol < 0) {
								c->vol = 0;
							}
							c->fade &= 1023;
						}
					}
				}
			}
			if (ins->pan_env.enabled) {
				int sus_x = 0;
				if (ins->pan_env.sustain < ins->pan_env.num_points) {
					sus_x = ins->pan_env.points[ins->pan_env.sustain][0];
				}
				if (c->key_on) {
					if (c->pan_env_tick < sus_x) {
						c->pan_env_tick++;
					} else {
						/* hold */
					}
				} else {
					c->pan_env_tick++;
				}
			}
		}
	}

	/* Next tick */
	st->tick++;
	if (st->tick >= st->speed) {
		st->tick = 0;
		/* advance row (end-of-song/back-jump detection is inside row advance) */
		pattern_control_after_row_advance(st);
	}
}

static void mix_block(xm_state_t *st, int16_t *out, size_t frames) {
	for (size_t s = 0; s < frames; s++) {
		if (--st->audio_tick <= 0) {
			engine_tick(st);
			st->audio_tick = st->audio_speed;
		}

		int64_t acc_l = 0;
		int64_t acc_r = 0;

		for (int ci = 0; ci < st->channels; ci++) {
			xm_channel_t *c = &st->ch[ci];
			if (!c->smp || !c->smp->pcm || c->sample_inc <= 0) {
				continue;
			}

			/* Ended sample without loop? */
			if (c->smp->loop_type == 0 && c->sample_pos >= (int) c->smp->length) {
				continue;
			}

			/* Handle loops */
			if (c->smp->loop_type == 1 && c->smp->loop_len > 1) {
				while (c->sample_pos >= (int) (c->smp->loop_start + c->smp->loop_len)) {
					c->sample_pos -= (int) c->smp->loop_len;
				}
			} else if (c->smp->loop_type == 2 && c->smp->loop_len > 1) {
				int loop_start = (int) c->smp->loop_start;
				int loop_end = loop_start + (int) c->smp->loop_len - 1;
				if (c->sample_pos > loop_end) {
					c->sample_pos = loop_end;
					c->loop_dir = -1;
				} else if (c->sample_pos < loop_start) {
					c->sample_pos = loop_start;
					c->loop_dir = 1;
				}
			}

			/* Interpolate */
			int pos = c->sample_pos;
			int next = pos + ((c->smp->loop_type == 2 && c->loop_dir < 0) ? -1 : 1);
			if (next < 0) {
				next = 0;
			}
			if (next >= (int) c->smp->length) {
				next = (int) c->smp->length - 1;
			}

			int16_t s1 = c->smp->pcm[pos];
			int16_t s2 = c->smp->pcm[next];
			uint32_t frac = (uint32_t) (c->sample_frac & 0xFFFF);
			int32_t s_lin = (int32_t) (((int64_t) s1 * (65536 - frac) + (int64_t) s2 * frac) >> 16);

			/* Effective volume with envelopes + tremolo */
			int vol = c->vol;
			if (c->trem_depth > 0) {
				static const int8_t trem_sine[64] = {
					0,12,25,37,49,60,71,81,90,98,105,111,116,120,123,125,
					126,125,123,120,116,111,105,98,90,81,71,60,49,37,25,12,
					0,-12,-25,-37,-49,-60,-71,-81,-90,-98,-105,-111,-116,-120,-123,-125,
					-126,-125,-123,-120,-116,-111,-105,-98,-90,-81,-71,-60,-49,-37,-25,-12
				};
				int w = trem_sine[c->trem_phase & 63];
				int dv = (w * c->trem_depth) >> 5;
				vol += dv;
				if (vol < 0) {
					vol = 0;
				}
				if (vol > 64) {
					vol = 64;
				}
			}

			/* Apply global volume */
			vol = (vol * st->global_vol) / 64;

			/* Stereo pan */
			int pan = c->pan; /* 0..255 */
			int32_t l = (s_lin * (255 - pan) * vol) >> 14; /* scale back to ~16-bit */
			int32_t r = (s_lin * pan * vol) >> 14;
			acc_l += l;
			acc_r += r;

			/* Advance */
			c->sample_frac += c->sample_inc;
			while (c->sample_frac >= 65536) {
				c->sample_frac -= 65536;
				c->sample_pos += (c->smp->loop_type == 2) ? c->loop_dir : 1;
			}
		}

		int32_t out_l = (int32_t) (acc_l >> 2); /* mix scaling */
		int32_t out_r = (int32_t) (acc_r >> 2);
		out_l = CLAMP(out_l, -32768, 32767);
		out_r = CLAMP(out_r, -32768, 32767);
		out[s * 2] = (int16_t) out_l;
		out[s * 2 + 1] = (int16_t) out_r;
	}
}

static void free_xm(xm_state_t *st) {
	if (st->patterns) {
		for (int i = 0; i < st->patterns_count; i++) {
			if (st->patterns[i].cells) {
				kfree(st->patterns[i].cells);
			}
		}
		kfree(st->patterns);
		st->patterns = NULL;
	}
	if (st->instruments) {
		for (int i = 0; i < st->instruments_count; i++) {
			xm_instrument_t *ins = &st->instruments[i];
			if (ins->samples) {
				for (int s = 0; s < ins->num_samples; s++) {
					if (ins->samples[s].pcm) {
						kfree(ins->samples[s].pcm);
					}
				}
				kfree(ins->samples);
			}
		}
		kfree(st->instruments);
		st->instruments = NULL;
	}
}

static bool xm_from_memory(const char *filename, const void *bytes, size_t len, void **out_ptr, size_t *out_bytes) {
	if (!filename || !bytes || len == 0 || !out_ptr || !out_bytes) {
		return false;
	}
	*out_ptr = NULL;
	*out_bytes = 0;

	if (!has_suffix_icase(filename, ".xm")) {
		return false;
	}

	xm_state_t st;
	if (!parse_xm_and_build(filename, (const uint8_t *) bytes, len, &st)) {
		dprintf("Couldn't parse as xm\n");
		free_xm(&st);
		return false;
	}

	/* Render in large blocks until ended or safety cap reached (30 minutes) */
	size_t cap_frames = 1024 * 1024 * 16;
	size_t used_frames = 0;
	int16_t *pcm = kmalloc(cap_frames * 2 * sizeof(int16_t));
	if (!pcm) {
		free_xm(&st);
		dprintf("xm: OOM initial buffer\n");
		return false;
	}

	const uint64_t max_total_frames = (uint64_t) XM_TARGET_RATE * 60 * 30;

	while (!st.ended && (uint64_t) used_frames < max_total_frames) {
		size_t want = 1024 * 1024 * 16;
		if (used_frames + want > cap_frames) {
			size_t new_cap = cap_frames * 2;
			if (new_cap < used_frames + want) {
				new_cap = used_frames + want;
			}
			int16_t *grown = krealloc(pcm, new_cap * 2 * sizeof(int16_t));
			if (!grown) {
				kfree(pcm);
				free_xm(&st);
				dprintf("xm: OOM grow buffer\n");
				return false;
			}
			pcm = grown;
			cap_frames = new_cap;
		}

		mix_block(&st, pcm + used_frames * 2, want);
		used_frames += want;
	}

	if (used_frames == 0) {
		kfree(pcm);
		free_xm(&st);
		dprintf("xm: empty output\n");
		return false;
	}

	int16_t *shrink = krealloc(pcm, used_frames * 2 * sizeof(int16_t));
	if (shrink) {
		pcm = shrink;
	}

	free_xm(&st);

	*out_ptr = pcm;
	*out_bytes = used_frames * 2 * sizeof(int16_t);
	return true;
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("xm: loaded\n");
	setforeground(COLOUR_LIGHTRED);
	kprintf("WARNING: The xm codec module is experimental. Here be dragons!\n");
	setforeground(COLOUR_WHITE);

	audio_file_loader_t *loader = (audio_file_loader_t *) kmalloc(sizeof(audio_file_loader_t));
	if (!loader) {
		dprintf("xm: failed to allocate loader\n");
		return false;
	}

	loader->next = NULL;
	loader->try_load_audio = xm_from_memory;
	loader->opaque = NULL;

	if (!register_audio_loader(loader)) {
		dprintf("xm: register_audio_loader failed\n");
		kfree(loader);
		return false;
	}

	g_xm_loader = loader;
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	if (g_xm_loader) {
		if (!deregister_audio_loader(g_xm_loader)) {
			kprintf("xm: deregister_audio_loader failed\n");
			return false;
		}
		kfree(g_xm_loader);
		g_xm_loader = NULL;
	}
	return true;
}