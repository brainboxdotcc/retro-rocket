/**
 * @file mod.c
 * @brief Retro Rocket MOD codec module (ProTracker/N-CHN/FLT# variants)
 *
 * This is a clean re-implementation for Retro Rocket,
 * providing full decode to 44.1 kHz S16_LE PCM, Inspired by ideas from
 * MODPlay (BSD-licensed, Martin Cameron et al.), but not derived from
 * its source code.
 *
 * Decodes ProTracker-style MOD files, and renders exactly one playthrough
 * and stops at the first loop marker:
 *  - any backward order jump (Bxx to a previous order), or
 *  - wrap from the last order back to the start.
 *
 * Notes / choices:
 *  - Supports common signatures at 0x438..0x43B (1080..1083):
 *      "M.K.", "M!K!", "FLT4".."FLT9", "4CHN","6CHN","8CHN","10CH","12CH","16CH","32CH"
 *    Other obscure signatures are not recognised here.
 *  - 31-sample ProTracker layout
 *  - 15-sample legacy module layout
 *  - Linear interpolation, classic Amiga-style panning pattern across channels.
 *  - 64-bit accumulators with explicit clipping to avoid overflow.
 *  - Input validation on pattern/sample regions to avoid OOB.
 */

#include <kernel.h>
#include "mod.h"

static audio_file_loader_t *g_mod_loader = NULL;

static uint32_t read_u32_be(const uint8_t *p) {
	return ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) | ((uint32_t) p[2] << 8) | (uint32_t) p[3];
}

static int is_mod_signature(const uint8_t *sig4, int *out_channels) {
	uint32_t sig = read_u32_be(sig4);

	/* 4-channel classics */
	if (sig == 0x4D2E4B2E /* "M.K." */ || sig == 0x4D214B21 /* "M!K!" */) {
		*out_channels = 4;
		return 1;
	}

	/* "FLT#"  */
	if ((sig & 0xFFFFFF00) == 0x464C5400) {
		int d = (int) (sig & 0xFF) - '0';
		if (d >= 4 && d <= 9) {
			*out_channels = d;
			return 1;
		}
	}

	/* "xCHN" (4..9CHN, 6CHN, 8CHN, 10CH,12CH,16CH,32CH) */
	if ((sig & 0x00FFFFFF) == 0x0043484E /* "\0CHN" */) {
		int tens = (int) ((sig >> 24) & 0xFF) - '0';
		if (tens >= 0 && tens <= 9) {
			*out_channels = tens;
			return 1;
		}
	}

	/* "xxCH" (10CH,12CH,16CH,32CH etc.) */
	if ((sig & 0x0000FFFF) == 0x00004348 /* "\0\0CH" */) {
		int t = (int) ((sig >> 24) & 0xFF) - '0';
		int o = (int) ((sig >> 16) & 0xFF) - '0';
		int v = t * 10 + o;
		if (v >= 10 && v <= MOD_MAX_CHANNELS) {
			*out_channels = v;
			return 1;
		}
	}

	return 0;
}

static void recalc_wave(osc_t *o, mod_state_t *st) {
	int32_t r = 0;
	switch (o->waveform) {
		case 0: {
			int idx = (int) (o->phase & 0x1F);
			r = sine_table[idx];
			if ((o->phase & 0x20) != 0) {
				r = -r;
			}
			break;
		}
		case 1: {
			r = 255 - (((o->phase + 0x20) & 0x3F) << 3);
			break;
		}
		case 2: {
			r = 255 - ((o->phase & 0x20) << 4);
			break;
		}
		case 3: {
			/* deterministic simple "randomness" */
			r = (int32_t) ((st->rng >> 20) & 0x3FF) - 255;
			st->rng = (st->rng * 65 + 17) & 0x1FFFFFFF;
			break;
		}
		default:
			r = 0;
			break;
	}
	o->val = r * (int32_t) o->depth;
}

static int32_t snap_period_to_semitone(int32_t base_note_period, int32_t current_period) {
	int32_t best = (int32_t)(( (int64_t)base_note_period * (int64_t)arpeggio_q16[0] ) >> 16);
	int32_t best_err = best - current_period;
	if (best_err < 0) {
		best_err = -best_err;
	}

	for (int k = 1; k < 12; k++) {
		int32_t p = (int32_t)(( (int64_t)base_note_period * (int64_t)arpeggio_q16[k] ) >> 16);
		int32_t e = p - current_period;
		if (e < 0) {
			e = -e;
		}
		if (e < best_err) {
			best_err = e;
			best = p;
		}
	}
	return best;
}

static void engine_tick(mod_state_t *st) {
	if (st->tick == 0) {
		st->skip_order_req = -1;

		for (int i = 0; i < st->channels; i++) {
			st->ch[i].vibrato.val = 0;
			st->ch[i].tremolo.val = 0;

			const uint8_t *cell = st->pattern_data
					      + MOD_NOTE_BYTES * (i + st->channels * (st->row + MOD_ROWS_PER_PATTERN * st->order_table[st->order]));

			int note_tmp = ((cell[0] << 8) | cell[1]) & 0x0FFF;
			int sample_tmp = (cell[0] & 0xF0) | (cell[2] >> 4);
			int eff_tmp = cell[2] & 0x0F;
			int effval_tmp = cell[3];

			if (st->ch[i].eff == 0 && st->ch[i].effval != 0) {
				st->ch[i].period = (int32_t) st->ch[i].note_period;
			}

			if (sample_tmp) {
				if (sample_tmp > MOD_SAMPLE_COUNT) {
					sample_tmp = 1;
				}
				st->ch[i].sample_idx = (uint8_t) (sample_tmp - 1);
				st->ch[i].gen.length = st->samples[sample_tmp - 1].actual_len;
				st->ch[i].gen.loop_length = st->samples[sample_tmp - 1].loop_len;
				st->ch[i].vol = ((const uint8_t *) (st->sample_hdrs + st->ch[i].sample_idx * MOD_SAMPLE_HDR_BYTES))[25]; /* volume at +25 */
				st->ch[i].gen.sample = st->samples[sample_tmp - 1].data;
			}

			if (note_tmp) {
				int finetune;
				if (eff_tmp == 0xE && (effval_tmp & 0xF0) == 0x50) {
					finetune = effval_tmp & 0x0F;
				} else {
					finetune = ((const uint8_t *) (st->sample_hdrs + st->ch[i].sample_idx * MOD_SAMPLE_HDR_BYTES))[24]; /* finetune at +24 */
				}

				note_tmp = (note_tmp * finetune_q16[finetune & 0x0F]) >> 16;
				st->ch[i].note_period = (uint32_t) note_tmp;

				if (eff_tmp != 0x3 && eff_tmp != 0x5 && (eff_tmp != 0xE || (effval_tmp & 0xF0) != 0xD0)) {
					st->ch[i].gen.age = 0;
					st->ch[i].gen.current_ptr = 0;
					st->ch[i].period = (int32_t) st->ch[i].note_period;

					if (st->ch[i].vibrato.waveform < 4) {
						st->ch[i].vibrato.phase = 0;
					}
					if (st->ch[i].tremolo.waveform < 4) {
						st->ch[i].tremolo.phase = 0;
					}
				}
			}

			if (eff_tmp || effval_tmp) {
				switch (eff_tmp) {
					case 0x3: {
						if (effval_tmp) {
							st->ch[i].slide_amount = (uint8_t) effval_tmp;
						}
						__attribute__((fallthrough));
					}
					case 0x5: {
						st->ch[i].slide_target = (int32_t) st->ch[i].note_period;
						break;
					}
					case 0x4: {
						if (effval_tmp & 0xF0) {
							st->ch[i].vibrato.speed = (uint8_t) (effval_tmp >> 4);
						}
						if (effval_tmp & 0x0F) {
							st->ch[i].vibrato.depth = (uint8_t) (effval_tmp & 0x0F);
						}
						/* fallthrough into 0x6 first tick calc */
						__attribute__((fallthrough));
					}
					case 0x6: {
						recalc_wave(&st->ch[i].vibrato, st);
						break;
					}
					case 0x7: {
						if (effval_tmp & 0xF0) {
							st->ch[i].tremolo.speed = (uint8_t) (effval_tmp >> 4);
						}
						if (effval_tmp & 0x0F) {
							st->ch[i].tremolo.depth = (uint8_t) (effval_tmp & 0x0F);
						}
						recalc_wave(&st->ch[i].tremolo, st);
						break;
					}
					case 0xC: {
						int v = (effval_tmp > 0x40) ? 0x40 : effval_tmp;
						st->ch[i].vol = (int16_t) v;
						break;
					}
					case 0x9: {
						if (effval_tmp) {
							st->ch[i].gen.current_ptr = (uint32_t) effval_tmp << 8;
							st->ch[i].sample_offset = (uint8_t) effval_tmp;
						} else {
							st->ch[i].gen.current_ptr = (uint32_t) st->ch[i].sample_offset << 8;
						}
						st->ch[i].gen.age = 0;
						break;
					}
					case 0xB: {
						if (effval_tmp >= st->orders) {
							effval_tmp = 0;
						}
						st->skip_order_req = effval_tmp;
						break;
					}
					case 0xD: {
						if (st->skip_order_req < 0) {
							if (st->order + 1 < st->orders) {
								st->skip_order_req = st->order + 1;
							} else {
								st->skip_order_req = 0;
							}
						}
						if (effval_tmp > 0x63) {
							effval_tmp = 0;
						}
						st->skip_order_dest_row = ((effval_tmp >> 4) * 10) + (effval_tmp & 0x0F);
						break;
					}
					case 0xE: {
						int x = effval_tmp >> 4;
						switch (x) {
							case 0x1:
								st->ch[i].period -= (effval_tmp & 0x0F);
								break;
							case 0x2:
								st->ch[i].period += (effval_tmp & 0x0F);
								break;
							case 0x4:
								st->ch[i].vibrato.waveform = (uint8_t) (effval_tmp & 0x07);
								break;
							case 0x6: {
								if (effval_tmp & 0x0F) {
									if (!st->pat_loop_cycle) {
										st->pat_loop_cycle = (effval_tmp & 0x0F) + 1;
									}
									if (st->pat_loop_cycle > 1) {
										st->skip_order_req = st->order;
										st->skip_order_dest_row = st->pat_loop_row;
									}
									st->pat_loop_cycle--;
								} else {
									st->pat_loop_row = st->row;
								}
								break;
							}
							case 0x7:
								st->ch[i].tremolo.waveform = (uint8_t) (effval_tmp & 0x07);
								break;
							case 0xA: {
								st->ch[i].vol += (effval_tmp & 0x0F);
								if (st->ch[i].vol > 0x40) {
									st->ch[i].vol = 0x40;
								}
								break;
							}
							case 0xB: {
								st->ch[i].vol -= (effval_tmp & 0x0F);
								if (st->ch[i].vol < 0) {
									st->ch[i].vol = 0;
								}
								break;
							}
							case 0xE: {
								st->max_tick *= ((effval_tmp & 0x0F) + 1);
								break;
							}
							default:
								break;
						}
						break;
					}
					case 0xF: {
						if (effval_tmp) {
							if (effval_tmp < 0x20) {
								st->max_tick = (st->max_tick / st->speed) * effval_tmp;
								st->speed = effval_tmp;
							} else {
								/* tempo */
								st->audio_speed = st->samplerate * 125 / (uint32_t) effval_tmp / 50;
							}
						}
						break;
					}
					default:
						break;
				}
			}

			st->ch[i].eff = (uint8_t) eff_tmp;
			st->ch[i].effval = (uint8_t) effval_tmp;
		}
	}

	/* per-tick effects and precalc step + volume */
	for (int i = 0; i < st->channels; i++) {
		int eff = st->ch[i].eff;
		int ev = st->ch[i].effval;

		if (eff || ev) {
			switch (eff) {
				case 0x0: {
					int m = st->tick % 3;
					if (m == 0) {
						st->ch[i].period = (int32_t) st->ch[i].note_period;
					} else if (m == 1) {
						st->ch[i].period = (int32_t) ((st->ch[i].note_period * arpeggio_q16[ev >> 4]) >> 16);
					} else {
						st->ch[i].period = (int32_t) ((st->ch[i].note_period * arpeggio_q16[ev & 0x0F]) >> 16);
					}
					break;
				}
				case 0x1: {
					if (st->tick) {
						st->ch[i].period -= ev;
					}
					break;
				}
				case 0x2: {
					if (st->tick) {
						st->ch[i].period += ev;
					}
					break;
				}
				case 0x5: {
					if (st->tick) {
						if (ev > 0x0F) {
							st->ch[i].vol += (ev >> 4);
							if (st->ch[i].vol > 0x40) {
								st->ch[i].vol = 0x40;
							}
						} else {
							st->ch[i].vol -= (ev & 0x0F);
							if (st->ch[i].vol < 0) {
								st->ch[i].vol = 0;
							}
						}
					}
					__attribute__((fallthrough));
					/* fallthrough to 0x3 */
				}
				case 0x3: {
					if (st->tick) {
						int amt = ev ? ev : st->ch[i].slide_amount;
						if (st->ch[i].slide_target > st->ch[i].period) {
							st->ch[i].period += amt;
							if (st->ch[i].slide_target < st->ch[i].period) {
								st->ch[i].period = st->ch[i].slide_target;
							}
						} else if (st->ch[i].slide_target < st->ch[i].period) {
							st->ch[i].period -= amt;
							if (st->ch[i].slide_target > st->ch[i].period) {
								st->ch[i].period = st->ch[i].slide_target;
							}
						}
						if (st->ch[i].glissando_on) {
							/* Snap to nearest semitone from the channel’s base note */
							int32_t base = (int32_t)st->ch[i].note_period;
							if (base > 0) {
								st->ch[i].period = snap_period_to_semitone(base, st->ch[i].period);
							}
						}
					}
					break;
				}
				case 0x4:
				case 0x6: {
					if (st->tick) {
						st->ch[i].vibrato.phase += st->ch[i].vibrato.speed;
						recalc_wave(&st->ch[i].vibrato, st);
					}
					break;
				}
				case 0xA: {
					if (st->tick) {
						if (ev > 0x0F) {
							st->ch[i].vol += (ev >> 4);
							if (st->ch[i].vol > 0x40) {
								st->ch[i].vol = 0x40;
							}
						} else {
							st->ch[i].vol -= (ev & 0x0F);
							if (st->ch[i].vol < 0) {
								st->ch[i].vol = 0;
							}
						}
					}
					break;
				}
				case 0x7: {
					if (st->tick) {
						st->ch[i].tremolo.phase += st->ch[i].tremolo.speed;
						recalc_wave(&st->ch[i].tremolo, st);
					}
					break;
				}
				case 0xE: {
					int x = ev >> 4;
					switch (x) {
						case 0x3: {
							st->ch[i].glissando_on = (uint8_t)((ev & 0x0F) ? 1 : 0);
							break;
						}
						case 0x9: {
							if (st->tick && !(st->tick % (ev & 0x0F))) {
								st->ch[i].gen.age = 0;
								st->ch[i].gen.current_ptr = 0;
								st->ch[i].gen.subptr_q16 = 0;
							}
							break;
						}
						case 0xC: {
							if (st->tick >= (ev & 0x0F)) {
								st->ch[i].vol = 0;
							}
							break;
						}
						case 0xD: {
							if (st->tick == (ev & 0x0F)) {
								st->ch[i].gen.age = 0;
								st->ch[i].gen.current_ptr = 0;
								st->ch[i].gen.subptr_q16 = 0;
								st->ch[i].period = (int32_t) st->ch[i].note_period;
							}
							break;
						}
						default:
							break;
					}
					break;
				}
				default:
					break;
			}
		}

		if (st->ch[i].period < 0 && st->ch[i].period != 0) {
			st->ch[i].period = 0;
		}

		if (st->ch[i].period != 0) {
			int32_t vib = st->ch[i].vibrato.val >> 7;
			int32_t p = st->ch[i].period + vib;
			if (p < 1) {
				p = 1;
			}
			st->ch[i].gen.period_q16 = (uint32_t) ((uint64_t) st->sample_rate_q16 / (uint32_t) p);
		} else {
			st->ch[i].gen.period_q16 = 0;
		}

		int32_t vol = st->ch[i].vol + (st->ch[i].tremolo.val >> 6);
		if (vol < 0) {
			vol = 0;
		}
		if (vol > 64) {
			vol = 64;
		}
		st->ch[i].gen.volume = vol;
	}

	st->tick++;
	if (st->tick >= st->max_tick) {
		st->tick = 0;
		st->max_tick = st->speed;

		if (st->skip_order_req >= 0) {
			/* loop/end detection: a backward jump ends the render */
			if (st->skip_order_req < st->order) {
				st->ended = true;
			}
			st->row = st->skip_order_dest_row;
			st->order = st->skip_order_req;
			st->skip_order_dest_row = 0;
			st->skip_order_req = -1;
		} else {
			st->row++;
			if (st->row >= MOD_ROWS_PER_PATTERN) {
				st->row = 0;
				int prev_order = st->order;
				st->order++;
				if (st->order >= st->orders) {
					/* wrap around = end */
					st->ended = true;
					st->order = 0;
				}
				if (st->order <= prev_order) {
					/* any backward movement = end */
					st->ended = true;
				}
			}
		}
	}
}

/* ==============================
   ===== Mixer / Renderer    =====
   ============================== */

static void mix_block(mod_state_t *st, int16_t *out, size_t frames) {
	/* panning weights: major/minor similar to original code */
	int32_t major_mul_q17 = 131072 / (st->channels / 2 ? st->channels / 2 : 1);
	int32_t minor_mul_q17 = (131072 / 3) / (st->channels / 2 ? st->channels / 2 : 1);

	for (size_t s = 0; s < frames; s++) {
		if (st->audio_tick == 0) {
			engine_tick(st);
			st->audio_tick = st->audio_speed;
		}
		st->audio_tick--;

		int64_t acc_l = 0;
		int64_t acc_r = 0;

		for (int ch = 0; ch < st->channels; ch++) {
			mod_channel_t *p = &st->ch[ch].gen;

			if (p->sample == NULL) {
				continue;
			}

			/* single-shot finished? */
			if (p->loop_length == 0 && p->current_ptr >= p->length) {
				continue;
			}

			/* loop wrap */
			while (p->current_ptr >= p->length) {
				if (p->loop_length != 0) {
					p->current_ptr -= p->loop_length;
				} else {
					p->current_ptr = p->length ? (p->length - 1) : 0;
					break;
				}
			}

			if (!p->muted) {
				/* linear interpolation */
				uint32_t next = p->current_ptr + 1;
				while (next >= p->length) {
					if (p->loop_length != 0) {
						next -= p->loop_length;
					} else {
						next = p->current_ptr;
						break;
					}
				}

				int32_t s1 = p->sample[p->current_ptr];
				int32_t s2 = p->sample[next];
				uint32_t frac = (uint32_t) p->subptr_q16 & 0xFFFFu;
				int32_t s_lin = (int32_t) ((((int64_t) s1 * (int64_t) (65536 - frac)) + ((int64_t) s2 * (int64_t) frac)) >> 16);

				int64_t scaled = (int64_t) s_lin * (int64_t) p->volume;

				if ((ch & 3) == 1 || (ch & 3) == 2) {
					acc_l += scaled * (int64_t) minor_mul_q17;
					acc_r += scaled * (int64_t) major_mul_q17;
				} else {
					acc_l += scaled * (int64_t) major_mul_q17;
					acc_r += scaled * (int64_t) minor_mul_q17;
				}
			}

			/* advance */
			p->subptr_q16 += (int32_t) p->period_q16;
			if (p->subptr_q16 >= 0x10000) {
				p->current_ptr += (uint32_t) (p->subptr_q16 >> 16);
				p->subptr_q16 &= 0xFFFF;
			}

			if (p->age < (uint32_t) 0x7FFFFFFF) {
				p->age++;
			}
		}

		/* scale back from q17 * volume (0..64) * 8-bit into 16-bit; empirically divide to avoid overflow */
		int32_t out_l = (int32_t) (acc_l >> 16);
		int32_t out_r = (int32_t) (acc_r >> 16);

		out_l = CLAMP(out_l, -32768, 32767);
		out_r = CLAMP(out_r, -32768, 32767);

		out[s * 2] = (int16_t) out_l;
		out[s * 2 + 1] = (int16_t) out_r;
	}
}

static bool parse_and_init(const char *name, const uint8_t *bytes, size_t len, mod_state_t *st) {
	memset(st, 0, sizeof(*st));
	st->file_bytes = bytes;
	st->file_len = len;
	st->samplerate = MOD_TARGET_RATE;
	st->sample_rate_q16 = (uint32_t) (((uint64_t) 3546895 << 16) / (uint64_t) st->samplerate);
	st->speed = 6;
	st->max_tick = 6;
	st->audio_speed = st->samplerate / 50;
	st->rng = 0x13579B; /* deterministic non-zero seed */

	if (len < MOD_HEADER_LEN) {
		return false;
	}

	if (!has_suffix_icase(name, ".mod")) {
		return false;
	}

	/* detect layout and channels */
	int channels = 0;
	bool is_legacy15 = false;

	if (is_mod_signature(bytes + MOD_SIG_OFFSET, &channels)) {
		is_legacy15 = false; /* ProTracker 31-sample layout */
	} else {
		/* Legacy 15-sample Soundtracker layout: no signature, fixed offsets, 4 ch */
		if (len >= 600) {
			channels = 4;
			is_legacy15 = true;
		} else {
			dprintf("15-sample legacy format unsupported or too short\n");
			return false;
		}
	}

	if (channels <= 0 || channels > MOD_MAX_CHANNELS) {
		dprintf("Too many channels: %d\n", channels);
		return false;
	}
	st->channels = channels;

	/* header offsets depend on layout */
	const uint8_t *sample_hdrs = bytes + 20;
	size_t sample_count = is_legacy15 ? 15 : MOD_SAMPLE_COUNT;
	size_t sample_hdr_region = sample_count * MOD_SAMPLE_HDR_BYTES;
	st->sample_hdrs = sample_hdrs;

	size_t order_count_off   = is_legacy15 ? 470 : MOD_ORDER_COUNT_OFFSET;
	size_t order_table_off   = is_legacy15 ? 472 : MOD_ORDER_TABLE_OFFSET;
	size_t pattern_region_off = is_legacy15
				    ? 600 /* 20 + 15*30 + 2 + 128, no 4-byte signature */
				    : (20 + sample_hdr_region + 2 + 128 + 4); /* = 1084 */

	if (!is_legacy15) {
		if (pattern_region_off != MOD_HEADER_LEN) {
			/* in ProTracker 31-sample layout this equals 1084 */
			dprintf("pattern_region_off (%lu) != MOD_HEADER_LEN (%u)\n", pattern_region_off, MOD_HEADER_LEN);
			return false;
		}
	}

	/* order count / order table */
	uint8_t order_count = bytes[order_count_off];
	if (order_count == 0 || order_count > MOD_MAX_ORDERS) {
		dprintf("Too many orders: %d\n", order_count);
		return false;
	}
	st->orders = order_count;
	st->order_table = bytes + order_table_off;

	/* find max pattern index present in order table */
	int maxpat = 0;
	for (int i = 0; i < MOD_MAX_ORDERS; i++) {
		uint8_t idx = st->order_table[i];
		if (idx > maxpat) {
			maxpat = idx;
		}
	}
	st->max_pattern = maxpat + 1;

	/* positions */
	st->pattern_data = bytes + pattern_region_off;

	/* compute size of pattern region */
	size_t pattern_bytes = (size_t) st->max_pattern * MOD_ROWS_PER_PATTERN * st->channels * MOD_NOTE_BYTES;

	if (len < pattern_region_off + pattern_bytes) {
		dprintf("(size_t)len < pattern_region_off + pattern_bytes\n");
		return false;
	}

	/* sample data follows immediately after pattern region */
	const int8_t *sample_mem = (const int8_t *) (bytes + pattern_region_off + pattern_bytes);
	const uint8_t *sample_end = bytes + len;

	/* parse sample headers: ONLY sample_count entries */
	for (size_t i = 0; i < sample_count; i++) {
		const uint8_t *sh = sample_hdrs + i * MOD_SAMPLE_HDR_BYTES;

		uint16_t len_words        = ((uint16_t) sh[22] << 8) | (uint16_t) sh[23];
		uint16_t loop_start_words = ((uint16_t) sh[26] << 8) | (uint16_t) sh[27];
		uint16_t loop_len_words   = ((uint16_t) sh[28] << 8) | (uint16_t) sh[29];

		uint32_t len_bytes        = (uint32_t) len_words * 2;
		uint32_t loop_start_bytes = (uint32_t) loop_start_words * 2;
		uint32_t loop_len_bytes   = (uint32_t) loop_len_words * 2;

		/* bounds on samplemem consumption */
		if ((const uint8_t *) sample_mem + len_bytes > sample_end) {
			dprintf("sample_mem + len past end\n");
			return false;
		}

		st->samples[i].data = sample_mem;
		st->samples[i].actual_len = len_bytes;
		if (loop_len_bytes < 2 || loop_start_bytes >= len_bytes || loop_start_bytes + loop_len_bytes > len_bytes) {
			st->samples[i].loop_len = 0;
		} else {
			st->samples[i].loop_len = loop_len_bytes;
		}

		sample_mem += len_bytes;
	}

	/* zero the remaining sample slots if legacy (entries 15..30) */
	for (size_t i = sample_count; i < (size_t)MOD_SAMPLE_COUNT; i++) {
		st->samples[i].data = NULL;
		st->samples[i].actual_len = 0;
		st->samples[i].loop_len = 0;
	}

	/* initialise channels */
	for (int i = 0; i < st->channels; i++) {
		st->ch[i].gen.age = 0x7FFFFFFF;
		st->ch[i].vol = 0x40;
		st->ch[i].glissando_on = 0;
	}

	/* extract module title */
	char mod_title[21];
	memcpy(mod_title, bytes, 20);
	mod_title[20] = 0;
	for (int i = 19; i >= 0; i--) {
		if (mod_title[i] != 0 && (mod_title[i] < 32 || mod_title[i] > 126)) {
			mod_title[i] = '?';
		} else if (mod_title[i] == ' ' || mod_title[i] == 0) {
			mod_title[i] = 0;
		} else {
			break;
		}
	}

	dprintf("Music module '%s': title: '%s' channels: %d orders: %d\n", name, mod_title, channels, order_count);
	return true;
}


static bool mod_from_memory(const char *filename, const void *bytes, size_t len, void **out_ptr, size_t *out_bytes) {
	if (!filename || !bytes || len == 0 || !out_ptr || !out_bytes) {
		return false;
	}
	*out_ptr = NULL;
	*out_bytes = 0;

	mod_state_t st;
	if (!parse_and_init(filename, (const uint8_t *) bytes, len, &st)) {
		dprintf("Couldn't parse as mod\n");
		return false;
	}

	/* render in blocks until st.ended */
	size_t cap_frames = 1024 * 1024 * 16; /* default 8mb */
	size_t used_frames = 0;
	int16_t *pcm = (int16_t *) kmalloc(cap_frames * 2 * sizeof(int16_t));
	if (!pcm) {
		dprintf("Failed to allocate initial blocks\n");
		return false;
	}

	/* safety cap: 30 minutes max */
	const uint64_t max_total_frames = (uint64_t) MOD_TARGET_RATE * 60 * 30;

	while (!st.ended && (uint64_t) used_frames < max_total_frames) {
		size_t want = 1024 * 1024 * 16;
		if (used_frames + want > cap_frames) {
			size_t new_cap = cap_frames * 2;
			if (new_cap < used_frames + want) {
				new_cap = used_frames + want;
			}
			int16_t *grown = (int16_t *) krealloc(pcm, new_cap * 2 * sizeof(int16_t));
			if (!grown) {
				kfree(pcm);
				dprintf("Out of memory\n");
				return false;
			}
			pcm = grown;
			cap_frames = new_cap;
		}

		mix_block(&st, pcm + used_frames * 2, want);
		used_frames += want;
	}

	/* shrink to fit */
	if (used_frames == 0) {
		kfree(pcm);
		dprintf("Empty file\n");
		return false;
	}

	int16_t *shrink = (int16_t *) krealloc(pcm, used_frames * 2 * sizeof(int16_t));
	if (shrink) {
		pcm = shrink;
	}

	*out_ptr = pcm;
	*out_bytes = used_frames * 2 * sizeof(int16_t);
	return true;
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("mod: loaded\n");

	audio_file_loader_t *loader = (audio_file_loader_t *) kmalloc(sizeof(audio_file_loader_t));
	if (!loader) {
		dprintf("mod: failed to allocate loader\n");
		return false;
	}

	loader->next = NULL;
	loader->try_load_audio = mod_from_memory;
	loader->opaque = NULL;

	if (!register_audio_loader(loader)) {
		dprintf("mod: register_audio_loader failed\n");
		kfree(loader);
		return false;
	}

	g_mod_loader = loader;
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	if (g_mod_loader) {
		if (!deregister_audio_loader(g_mod_loader)) {
			kprintf("mod: deregister_audio_loader failed\n");
			return false;
		}
		kfree(g_mod_loader);
		g_mod_loader = NULL;
	}
	return true;
}
