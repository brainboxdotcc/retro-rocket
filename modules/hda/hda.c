/**
 * @file modules/hda/hda.c
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 *
 * Intel High Definition Audio (Azalia) output driver
 *
 * References:
 *  - Intel High Definition Audio Specification Rev. 1.0a
 *    (section 3.3.* stream regs, section 3.4 ICOI/ICII/ICIS,
 *    CORB/RIRB, SDnFMT encoding, BDL alignment) and widget verbs
 *  - OSDev HDA overview for register map & verb list
 */
#include <kernel.h>
#include "hda.h"

static hda_dev_t hda;
static int16_t *s_q_pcm = NULL;
static size_t s_q_cap_fr = 0, s_q_len_fr = 0, s_q_head_fr = 0;
static bool s_paused = false;

/**
 * @brief Cached list of discoverable output pins (per codec)
 */
static struct {
	uint8_t pin;	/**< Pin NID */
	uint8_t dac;	/**< DAC NID reachable from this pin */
	char label[64];	/**< e.g. "Front Headphone (3.5mm Jack)" */
} s_hda_outputs[16];

static size_t s_hda_outputs_n = 0;

static inline uint8_t hda_mmio_r8(uint32_t off) {
	return *((volatile uint8_t *) (uintptr_t) (hda.mmio + off));
}

static inline uint16_t hda_mmio_r16(uint32_t off) {
	return *((volatile uint16_t *) (uintptr_t) (hda.mmio + off));
}

static inline uint32_t hda_mmio_r32(uint32_t off) {
	return *((volatile uint32_t *) (uintptr_t) (hda.mmio + off));
}

static inline void hda_mmio_w8(uint32_t off, uint8_t v) {
	*((volatile uint8_t *) (uintptr_t) (hda.mmio + off)) = v;
}

static inline void hda_mmio_w16(uint32_t off, uint16_t v) {
	*((volatile uint16_t *) (uintptr_t) (hda.mmio + off)) = v;
}

static inline void hda_mmio_w32(uint32_t off, uint32_t v) {
	*((volatile uint32_t *) (uintptr_t) (hda.mmio + off)) = v;
}

static inline void hda_mmio_fence(void) {
	hda_mmio_r32(HDA_REG_GCTL);
}

/**
 * @brief Pause for a few microseconds
 */
static inline void tiny_delay(void) {
	for (int i = 0; i < 64; i++) {
		io_wait();
	}
}

/**
 * @brief Set up CORB/RIRB (ring buffers)
 * leaves rings enabled but driver is free to use immediate verbs too.
 */
static bool hda_corb_rirb_init(void) {
	hda_mmio_w8(HDA_REG_CORBCTL, 0);
	hda_mmio_w8(HDA_REG_RIRBCTL, 0);

	uint8_t corbsz = hda_mmio_r8(HDA_REG_CORBSIZE);
	uint8_t rirbsz = hda_mmio_r8(HDA_REG_RIRBSIZE);
	if (!(corbsz & (1 << 6)) || !(rirbsz & (1 << 6))) {
		dprintf("hda: controller lacks 256-entry CORB/RIRB support\n");
		return false;
	}
	hda_mmio_w8(HDA_REG_CORBSIZE, (hda_mmio_r8(HDA_REG_CORBSIZE) & ~0x03) | 0x02);
	hda_mmio_w8(HDA_REG_RIRBSIZE, (hda_mmio_r8(HDA_REG_RIRBSIZE) & ~0x03) | 0x02);

	size_t corb_bytes = 256 * sizeof(uint64_t);
	size_t rirb_bytes = 256 * sizeof(uint64_t);
	void *corb_raw = kmalloc_low(corb_bytes + 1024);
	void *rirb_raw = kmalloc_low(rirb_bytes + 1024);
	if (!corb_raw || !rirb_raw) {
		dprintf("hda: OOM allocating CORB/RIRB\n");
		return false;
	}
	uintptr_t corb_al = ((uintptr_t) corb_raw + 1023) & ~(uintptr_t) 1023;
	uintptr_t rirb_al = ((uintptr_t) rirb_raw + 1023) & ~(uintptr_t) 1023;

	hda.corb = (uint64_t *) corb_al;
	hda.rirb = (uint64_t *) rirb_al;
	hda.corb_phys = (uint32_t) corb_al;
	hda.rirb_phys = (uint32_t) rirb_al;
	hda.corb_entries = 256;
	hda.rirb_entries = 256;

	memset(hda.corb, 0, corb_bytes);
	memset(hda.rirb, 0, rirb_bytes);

	/* program bases */
	hda_mmio_w32(HDA_REG_CORBLBASE, hda.corb_phys);
	hda_mmio_w32(HDA_REG_CORBUBASE, 0);
	hda_mmio_w16(HDA_REG_CORBRP, HDA_CORBRP_RST);
	for (int i = 0; i < 1000; i++) {
		if (hda_mmio_r16(HDA_REG_CORBRP) & HDA_CORBRP_RST) {
			break;
		}
		tiny_delay();
	}
	hda_mmio_w16(HDA_REG_CORBRP, 0);

	hda_mmio_w32(HDA_REG_RIRBLBASE, hda.rirb_phys);
	hda_mmio_w32(HDA_REG_RIRBUBASE, 0);
	hda_mmio_w16(HDA_REG_RIRBWP, HDA_RIRBWP_RST); /* write 1 to reset pointer */

	/* one response per entry */
	hda_mmio_w16(HDA_REG_RINTCNT, 1);

	/* enable DMA (interrupts optional; we poll) */
	hda_mmio_w8(HDA_REG_CORBCTL, HDA_CORBCTL_DMAE);
	hda_mmio_w8(HDA_REG_RIRBCTL, HDA_RIRBCTL_DMAE);

	return true;
}

/**
 * @brief Issue a verb; prefer CORB/RIRB but fall back to Immediate Command if no response arrives.
 * Uses WALLCLK as a timebase, so it’s robust without interrupts/timers.
 */
static bool hda_cmd(uint8_t cad, uint8_t nid, uint16_t verb, uint16_t payload, uint32_t *resp_out) {
	/* Fast path: CORB/RIRB */
	if (hda.corb && hda.rirb) {
		uint32_t wp = hda_mmio_r16(HDA_REG_CORBWP) & 0xFF;
		uint32_t next = (wp + 1) & 0xFF;
		uint32_t old_rirb_wp = hda_mmio_r16(HDA_REG_RIRBWP) & 0xFF;

		uint32_t v = HDA_MAKE_VERB(cad, nid, verb, payload);
		hda.corb[next] = (uint64_t) v;
		hda_mmio_w16(HDA_REG_CORBWP, (uint16_t) next);
		hda_mmio_fence(); /* ensure CORBWP is visible to the device thread */

		/* Wait up to ~5 ms using WALLCLK (=24.576 MHz) */
		uint32_t start = hda_mmio_r32(HDA_REG_WALCLK);
		uint32_t deadline = start + (5 * WALCLK_HZ);

		for (;;) {
			uint32_t nw = hda_mmio_r16(HDA_REG_RIRBWP) & 0xFF;
			if (nw != old_rirb_wp) {
				uint64_t r = hda.rirb[nw];
				if (resp_out) {
					*resp_out = (uint32_t) (r & 0xFFFFFFFF);
				}
				return true;
			}
			uint32_t now = hda_mmio_r32(HDA_REG_WALCLK);
			if ((int32_t) (now - deadline) >= 0) {
				break; /* timeout → fall back to immediate */
			}
			__asm__ __volatile__("pause");
		}
	}

	/* Fallback: Immediate Command (ICOI/ICII/ICIS) */
	const uint32_t REG_ICOI = 0x60, REG_ICII = 0x64, REG_ICIS = 0x68; /* spec section 3.3.12 */
	const uint8_t ICIS_ICB = 1 << 0; /* busy */
	const uint8_t ICIS_IRV = 1 << 1; /* response valid */

	/* wait for not-busy (≤5 ms) */
	{
		uint32_t start = hda_mmio_r32(HDA_REG_WALCLK);
		uint32_t deadline = start + (5 * WALCLK_HZ);
		while (hda_mmio_r8(REG_ICIS) & ICIS_ICB) {
			uint32_t now = hda_mmio_r32(HDA_REG_WALCLK);
			if ((int32_t) (now - deadline) >= 0) {
				break;
			}
			__asm__ __volatile__("pause");
		}
	}

	/* write verb */
	uint32_t v = HDA_MAKE_VERB(cad, nid, verb, payload);
	hda_mmio_w32(REG_ICOI, v);

	/* kick: set ICB=1, then fence */
	hda_mmio_w8(REG_ICIS, ICIS_ICB);
	hda_mmio_fence();

	/* wait for response valid (≤5 ms) */
	{
		uint32_t start = hda_mmio_r32(HDA_REG_WALCLK);
		uint32_t deadline = start + (5 * WALCLK_HZ);
		for (;;) {
			uint8_t s = hda_mmio_r8(REG_ICIS);
			if (s & ICIS_IRV) {
				uint32_t resp = hda_mmio_r32(REG_ICII);
				/* clear IRV by writing 1, then fence */
				hda_mmio_w8(REG_ICIS, ICIS_IRV);
				hda_mmio_fence();
				if (resp_out) {
					*resp_out = resp;
				}
				return true;
			}
			uint32_t now = hda_mmio_r32(HDA_REG_WALCLK);
			if ((int32_t) (now - deadline) >= 0) {
				break;
			}
			__asm__ __volatile__("pause");
		}
	}

	dprintf("hda: verb 0x%03x to cad %u nid %u timed out (both paths)\n", verb, cad, nid);
	return false;
}

/**
 * @brief GET_PARAMETER helper returning 32-bit value
 */
static inline bool hda_get_param(uint8_t cad, uint8_t nid, uint8_t param, uint32_t *out) {
	return hda_cmd(cad, nid, V_GET_PARAM, param, out);
}

/**
 * @brief Decode a Pin's Default Configuration (V_GET_DEF_CFG) into a human-readable label.
 *
 * Examples:
 *  - "Rear Line Out (3.5mm Jack)"
 *  - "Front Headphone (3.5mm Jack)"
 *  - "Internal Speaker"
 *  - "Rear SPDIF (Optical)"
 *
 * @param defcfg  32-bit default configuration value from V_GET_DEF_CFG.
 * @param buf     Output buffer for the assembled label string.
 * @param n       Size of @p buf in bytes.
 * @return number of characters written (excluding the terminating NUL).
 */
static size_t hda_pin_label(uint32_t defcfg, char *buf, size_t n) {
	/* Field extractors (per Intel HDA spec) */
	const uint8_t dev = (uint8_t) ((defcfg >> 20) & 0x0F);   /* default device (23:20) */
	const uint8_t conn = (uint8_t) ((defcfg >> 16) & 0x0F);   /* connection type (19:16) */
	const uint8_t loc6 = (uint8_t) ((defcfg >> 24) & 0x3F);   /* location (29:24); bit4 = internal flag */
	const uint8_t cty = (uint8_t) ((defcfg >> 30) & 0x03);   /* connectivity (31:30): 0=Jack,2=Fixed */

	/* Device name */
	const char *dev_name = "Output";
	switch (dev) {
		case 0x01:
			dev_name = "Speaker";
			break; /* DEFDEV_SPEAKER */
		case 0x02:
			dev_name = "Line Out";
			break; /* DEFDEV_LINEOUT */
		case 0x0F:
			dev_name = "Headphone";
			break; /* DEFDEV_HP_OUT  */
			/* common digital variants */
		case 0x03: /* fallthrough */
		case 0x05:
			dev_name = "SPDIF";
			break;
		case 0x04:
			dev_name = "Digital";
			break;
		default:
			/* dev == 0 (unspecified): choose a sensible default */
			if (((defcfg >> 30) & 0x3) == 2 /* Fixed */ || ((defcfg >> 24) & 0x10)) {
				dev_name = "Speaker";  /* internal/fixed output → speaker */
			} else {
				dev_name = "Line Out"; /* external jack with unknown dev → line out */
			}
			break;
	}

	/* Location name */
	/* loc6: [5]=internal(1)/external(0), [3:0]=place (0:NA,1:Rear,2:Front,3:Left,4:Right,5:Top,6:Bottom,7:Special) */
	const int is_internal = (loc6 & 0x10) ? 1 : 0;
	const uint8_t place = (uint8_t) (loc6 & 0x0F);
	const char *place_name = NULL;

	if (is_internal) {
		/* Internal devices are usually "Internal <Device>" or "Internal <place> <Device>" */
		switch (place) {
			case 0x00:
				place_name = "Internal";
				break;
			case 0x01:
				place_name = "Internal Rear";
				break;
			case 0x02:
				place_name = "Internal Front";
				break;
			default:
				place_name = "Internal";
				break;
		}
	} else {
		switch (place) {
			case 0x01:
				place_name = "Rear";
				break;
			case 0x02:
				place_name = "Front";
				break;
			case 0x03:
				place_name = "Left";
				break;
			case 0x04:
				place_name = "Right";
				break;
			case 0x05:
				place_name = "Top";
				break;
			case 0x06:
				place_name = "Bottom";
				break;
			default:
				place_name = NULL;
				break;
		}
	}

	/* Connector description */
	/* Connection type (19:16) — keep to the common, recognisable ones. */
	const char *conn_desc = NULL;
	switch (conn) {
		case 0x01:
			conn_desc = "3.5mm Jack"; /* Standard headphone */
			break;
		case 0x02:
			conn_desc = "6.35mm Jack"; /* Fat headphone/amplifier */
			break;
		case 0x04:
			conn_desc = "RCA";
			break;
		case 0x05:
			conn_desc = "Optical"; /* TOSLINK */
			break;
		case 0x06:
			conn_desc = "Digital"; /* generic electrical digital */
			break;
		case 0x07:
			conn_desc = "Analogue";
			break;
		case 0x11:
			conn_desc = "ATAPI"; /* some codecs report this */
			break;
		default:
			conn_desc = NULL;
			break;
	}

	/* Connectivity (31:30): 0=Jack, 1=No conn, 2=Fixed, 3=Both */
	const int is_jack = (cty == 0);
	const int is_fixed = (cty == 2);

	/* If it’s a jack, and we have a type, prefer "(type)"; else, "(Jack)" for clarity */
	const char *suffix = NULL;
	char jack_fallback[16];
	if (is_jack) {
		if (conn_desc) {
			suffix = conn_desc;
		} else {
			/* generic "Jack" if we know it is a jack but not the subtype */
			jack_fallback[0] = 0;
			snprintf(jack_fallback, sizeof(jack_fallback), "Jack");
			suffix = jack_fallback;
		}
	} else if (is_fixed && conn_desc) {
		suffix = conn_desc; /* e.g. "Optical" on a fixed SPDIF port */
	}

	/* Aim for concise labels; only include components we actually know. */
	if (place_name && suffix) {
		return snprintf(buf, n, "%s %s (%s)", place_name, dev_name, suffix);
	} else if (place_name) {
		return snprintf(buf, n, "%s %s", place_name, dev_name);
	} else if (suffix) {
		return snprintf(buf, n, "%s (%s)", dev_name, suffix);
	}
	return snprintf(buf, n, "%s", dev_name);
}


/* Unmute and set safe 0 dB gain on the selected DAC and Pin (both channels).
 * HDA Set Amp Gain/Mute (verb 0x003) payload:
 * [15]=Mute, [12:8]=Gain step, [7]=Update, [6:4]=Index(0), [3]=Dir (0=Output,1=Input), [2:0]=Channel (7=Both)
 * We request: output dir, both channels, update=1, mute=0, gain=0 (0 dB on most codecs). */
static void hda_unmute_codec_path(void) {
	const uint16_t AMP_PAYLOAD_UNMUTE_0DB =
		(0 << 15) |  /* mute=0 */
		(0 << 8) |   /* gain=0 */
		(1 << 7) |   /* update=1 */
		(0 << 4) |   /* index=0 */
		(0 << 3) |   /* dir=output */
		7;           /* channels=both */

	/* Best-effort: some widgets may lack amps; verbs will be ignored if unsupported. */
	hda_cmd(hda.cad, hda.dac_nid, V_SET_AMP_GAIN_MUTE, AMP_PAYLOAD_UNMUTE_0DB, NULL);
	hda_cmd(hda.cad, hda.pin_nid, V_SET_AMP_GAIN_MUTE, AMP_PAYLOAD_UNMUTE_0DB, NULL);
}

static bool hda_find_afg(uint8_t cad, uint8_t *afg_nid, nid_range_t *widgets) {
	uint32_t func_range;
	if (!hda_get_param(cad, 0x00, P_NODE_COUNT, &func_range)) {
		return false;
	}
	uint8_t fn_start = (uint8_t) ((func_range >> 16) & 0xFF);
	uint8_t fn_count = (uint8_t) (func_range & 0xFF);
	for (uint8_t i = 0; i < fn_count; i++) {
		uint8_t fn = fn_start + i;
		uint32_t ftype;
		if (!hda_get_param(cad, fn, P_FUNC_GRP_TYPE, &ftype)) {
			continue;
		}
		if ((ftype & 0xFF) == 0x01) { /* AFG */
			uint32_t w = 0;
			if (!hda_get_param(cad, fn, P_NODE_COUNT, &w)) {
				continue;
			}
			widgets->start_nid = (uint8_t) ((w >> 16) & 0xFF);
			widgets->count = (uint8_t) (w & 0xFF);
			*afg_nid = fn;
			return true;
		}
	}
	return false;
}

static bool hda_widget_type(uint8_t cad, uint8_t nid, uint8_t *type_out, uint32_t *caps_out) {
	uint32_t caps;
	if (!hda_get_param(cad, nid, P_AW_CAPS, &caps)) {
		return false;
	}
	if (caps_out) {
		*caps_out = caps;
	}
	if (type_out) {
		*type_out = (uint8_t) ((caps >> 20) & 0x0F);
	}
	return true;
}

/* Read the connection list of 'nid' into 'out', return count. Handles both short and long forms.
 * Returns 0 if none or on error. Max entries bounded by out_cap. */
static uint32_t hda_get_conn_list(uint8_t cad, uint8_t nid, uint8_t *out, uint32_t out_cap) {
	uint32_t clen;
	uint8_t param = 0x0E;
	if (!hda_get_param(cad, nid, param, &clen)) {
		return 0;
	}
	/* Per spec/OSDev:
	   - bit 7 (of low byte) indicates 'long form' (1 = long form)
	   - low 7 bits are the number of entries
	   - GET_CONN_LIST (0xF02) returns either one 8-bit entry (short form),
	     or up to 4 entries packed in 32 bits (long form) starting at index&~3. */
	uint8_t long_form = (uint8_t) ((clen & 0x80) ? 1 : 0);
	uint32_t n = clen & 0x7F;
	if (n == 0 || out_cap == 0) {
		return 0;
	}
	if (n > out_cap) {
		n = out_cap;
	}

	if (!long_form) {
		/* short form: one entry per verb */
		for (uint32_t i = 0; i < n; i++) {
			uint32_t r;
			if (!hda_cmd(cad, nid, V_GET_CONN_LIST, (uint16_t) i, &r)) {
				return i;
			}
			out[i] = (uint8_t) (r & 0xFF);
		}
		return n;
	} else {
		/* long form: verb index rounded down to multiple of 4; response packs 4 entries (b0..b3) */
		uint32_t got = 0;
		for (uint32_t i = 0; i < n;) {
			uint16_t base = (uint16_t) (i & ~3);
			uint32_t r;
			if (!hda_cmd(cad, nid, V_GET_CONN_LIST, base, &r)) {
				return got;
			}
			uint8_t e0 = (uint8_t) ((r >> 0) & 0xFF);
			uint8_t e1 = (uint8_t) ((r >> 8) & 0xFF);
			uint8_t e2 = (uint8_t) ((r >> 16) & 0xFF);
			uint8_t e3 = (uint8_t) ((r >> 24) & 0xFF);
			uint8_t pack[4] = {e0, e1, e2, e3};
			for (uint32_t k = 0; k < 4 && i < n; k++, i++) {
				out[i] = pack[k];
				got++;
			}
		}
		return got;
	}
}

/* Depth-limited search for an Audio Output (DAC) reachable from 'start_nid'.
   Accepts direct DACs and those behind a Selector/Mixer. Prevents simple cycles. */
static bool hda_find_dac_downstream(uint8_t cad, uint8_t start_nid, uint8_t *out_dac) {
	uint8_t visited[256];
	memset(visited, 0, sizeof(visited));

	uint8_t stack[64];
	int sp = 0;
	stack[sp++] = start_nid;

	while (sp > 0) {
		uint8_t nid = stack[--sp];

		if (visited[nid]) {
			continue;
		}
		visited[nid] = 1;

		uint8_t wtype;
		uint32_t wcaps;
		if (!hda_widget_type(cad, nid, &wtype, &wcaps)) {
			continue;
		}

		if (wtype == WTYPE_AUDIO_OUT) {
			*out_dac = nid;
			return true;
		}

		/* Traverse mixers/selectors only; other widget types are not fanned out here. */
		if (wtype == WTYPE_MIXER || wtype == WTYPE_SELECTOR) {
			uint8_t peers[64];
			uint32_t n_peers = hda_get_conn_list(cad, nid, peers, (uint32_t) sizeof(peers));
			for (uint32_t i = 0; i < n_peers && sp < (int) sizeof(stack); i++) {
				uint8_t peer = peers[i];
				if (!visited[peer]) {
					stack[sp++] = peer;
				}
			}
		}
	}

	return false;
}

/* Choose an output DAC and an output Pin, and build a cache of all candidate outputs.
 * Supports Pin -> (Selector/Mixer)* -> DAC, not just direct Pin -> DAC.
 * Prefers default devices: SPEAKER, LINEOUT, then HP; otherwise falls back.
 * Also fills s_hda_outputs[] with (pin,dac,label) for later selection-by-name.
 */
static bool hda_pick_output_path(uint8_t cad, nid_range_t wr, uint8_t *out_dac, uint8_t *out_pin) {
	s_hda_outputs_n = 0; /* rebuild cache */

	uint8_t best_pin = 0, best_dac = 0;
	uint8_t fallback_pin = 0, fallback_dac = 0;

	for (uint8_t n = 0; n < wr.count; n++) {
		uint8_t pin = (uint8_t) (wr.start_nid + n);

		uint8_t wtype;
		uint32_t wcaps;
		if (!hda_widget_type(cad, pin, &wtype, &wcaps)) continue;
		if (wtype != WTYPE_PIN_COMPLEX) continue;

		uint32_t pincaps;
		if (!hda_get_param(cad, pin, P_PIN_CAPS, &pincaps)) continue;
		if ((pincaps & PINCAP_OUT) == 0) continue; /* not output-capable */

		uint32_t defcfg = 0;
		(void) hda_cmd(cad, pin, V_GET_DEF_CFG, 0, &defcfg);
		uint8_t dev = (uint8_t) ((defcfg >> 20) & 0x0F);

		/* Find a reachable DAC (direct or via selector/mixer) */
		uint8_t peers[64];
		uint32_t n_peers = hda_get_conn_list(cad, pin, peers, (uint32_t) sizeof(peers));

		uint8_t found_dac = 0;
		for (uint32_t i = 0; i < n_peers && !found_dac; i++) {
			uint8_t peer = peers[i];
			uint8_t ptype;
			uint32_t pcaps;
			if (!hda_widget_type(cad, peer, &ptype, &pcaps)) continue;

			if (ptype == WTYPE_AUDIO_OUT) {
				found_dac = peer;
			} else if (ptype == WTYPE_MIXER || ptype == WTYPE_SELECTOR) {
				uint8_t via = 0;
				if (hda_find_dac_downstream(cad, peer, &via)) {
					found_dac = via;
				}
			}
		}

		/* If we found a usable path, add it to the outputs cache with its label */
		if (found_dac) {
			if (s_hda_outputs_n < (sizeof(s_hda_outputs) / sizeof(s_hda_outputs[0]))) {
				char name[64];
				hda_pin_label(defcfg, name, sizeof(name));
				s_hda_outputs[s_hda_outputs_n].pin = pin;
				s_hda_outputs[s_hda_outputs_n].dac = found_dac;
				/* ensure NUL-terminated copy */
				size_t j = 0;
				for (; j + 1 < sizeof(s_hda_outputs[0].label) && name[j]; ++j) s_hda_outputs[s_hda_outputs_n].label[j] = name[j];
				s_hda_outputs[s_hda_outputs_n].label[j] = '\0';
				s_hda_outputs_n++;
			}

			/* Selection policy: prefer Speaker > LineOut > Headphone; otherwise remember as fallback */
			if (!best_pin) {
				if (dev == DEFDEV_SPEAKER || dev == DEFDEV_LINEOUT || dev == DEFDEV_HP_OUT) {
					best_pin = pin;
					best_dac = found_dac;
				} else if (!fallback_pin) {
					fallback_pin = pin;
					fallback_dac = found_dac;
				}
			}
		}
	}

	/* Choose best, else fallback, else fail */
	if (best_pin && best_dac) {
		*out_pin = best_pin;
		*out_dac = best_dac;
		return true;
	}
	if (fallback_pin && fallback_dac) {
		*out_pin = fallback_pin;
		*out_dac = fallback_dac;
		return true;
	}
	return false;
}

/* Rebind the active output path to the cached entry whose label matches 'name' (case-insensitive).
 * Returns true on success (path switched or already active), false if no such name is cached.
 * Safe to call at any time after initial bring-up; the outputs cache is built during pick.
 */
static bool hda_select_output_by_name(const char *name) {
	if (!name || s_hda_outputs_n == 0) {
		return false;
	}

	/* Find the requested entry */
	size_t idx = (size_t) -1;
	for (size_t i = 0; i < s_hda_outputs_n; i++) {
		if (strcasecmp(s_hda_outputs[i].label, name) == 0) {
			idx = i;
			break;
		}
	}
	if (idx == (size_t) -1) {
		return false; /* name not found */
	}

	uint8_t new_pin = s_hda_outputs[idx].pin;
	uint8_t new_dac = s_hda_outputs[idx].dac;

	/* If already selected, nothing to do */
	if (hda.pin_nid == new_pin && hda.dac_nid == new_dac) {
		return true;
	}

	/* Politely disable previous pin's output amp (best-effort) */
	if (hda.pin_nid) {
		const uint16_t AMP_MUTE_BOTH =
			(1 << 15) | /* mute=1 */
			(1 << 7) | /* update */
			(0 << 4) | /* index=0 */
			(0 << 3) | /* dir=output */
			7;          /* channels=both */
		hda_cmd(hda.cad, hda.pin_nid, V_SET_AMP_GAIN_MUTE, AMP_MUTE_BOTH, NULL);
		/* clear OUT_EN */
		hda_cmd(hda.cad, hda.pin_nid, V_SET_PIN_WCTRL, 0x00, NULL);
	}

	/* Power up and bind new path */
	hda_cmd(hda.cad, new_pin, V_SET_POWER_STATE, 0x00, NULL); /* D0 */
	hda_cmd(hda.cad, new_dac, V_SET_POWER_STATE, 0x00, NULL); /* D0 */

	/* Bind converter to our output stream tag; channel 0 (left). */
	hda_cmd(hda.cad, new_dac, V_SET_CONV_STREAMCH, (uint16_t) ((hda.out_stream_tag << 4) | 0x0), NULL);
	/* Stereo channels (value = n-1) */
	hda_cmd(hda.cad, new_dac, V_SET_CHANNEL_COUNT, 1, NULL);

	/* Enable the new pin: OUT_EN plus EAPD if available */
	hda_cmd(hda.cad, new_pin, V_SET_PIN_WCTRL, 0x40, NULL); /* OUT_EN */
	hda_cmd(hda.cad, new_pin, V_SET_EAPD_BTL, 0x02, NULL);  /* EAPD */

	/* Update current selection and unmute */
	hda.pin_nid = new_pin;
	hda.dac_nid = new_dac;
	hda_unmute_codec_path();

	dprintf("hda: output switched to Pin 0x%02x: %s (DAC 0x%02x)\n", new_pin, s_hda_outputs[idx].label, new_dac);
	return true;
}

static const char **hda_list_output_names(void) {
	static const char *names[sizeof(s_hda_outputs) / sizeof(s_hda_outputs[0]) + 1];
	for (size_t i = 0; i < s_hda_outputs_n; i++) {
		names[i] = s_hda_outputs[i].label;
	}
	names[s_hda_outputs_n] = NULL;
	return names;
}

static const char *hda_get_current_output(void) {
	uint32_t defcfg = 0;
	if (hda_cmd(hda.cad, hda.pin_nid, V_GET_DEF_CFG, 0, &defcfg)) {
		static char output_label[64];
		hda_pin_label(defcfg, output_label, sizeof(output_label));
		return output_label;
	}
	return "";
}

static void hda_dump_topology(uint8_t cad, uint8_t afg, nid_range_t wr) {
	dprintf("HDA topology: cad=%u afg=0x%02x nodes [%u..%u]", cad, afg, wr.start_nid, (uint8_t) (wr.start_nid + wr.count - 1));
	for (uint8_t n = 0; n < wr.count; n++) {
		uint8_t nid = wr.start_nid + n;
		uint8_t t;
		uint32_t c;
		if (!hda_widget_type(cad, nid, &t, &c)) {
			continue;
		}
		if (t == WTYPE_PIN_COMPLEX) {
			uint32_t pc;
			hda_get_param(cad, nid, P_PIN_CAPS, &pc);
			uint32_t cfg = 0;
			hda_cmd(cad, nid, V_GET_DEF_CFG, 0, &cfg);
			uint8_t dev = (uint8_t) ((cfg >> 20) & 0x0F);
			dprintf("NID 0x%02x: PIN caps=0x%08x defdev=%u", nid, pc, dev);

			uint8_t peers[64];
			uint32_t n_peers = hda_get_conn_list(cad, nid, peers, (uint32_t) sizeof(peers));
			for (uint32_t i = 0; i < n_peers; i++) {
				uint8_t p = peers[i];
				uint8_t pt;
				uint32_t pc2;
				if (hda_widget_type(cad, p, &pt, &pc2)) {
					dprintf("    -> 0x%02x type=%u", p, pt);
				}
			}
		} else {
			dprintf("NID 0x%02x: type=%u caps=0x%08lx", nid, t, (unsigned long) c);
		}
	}
}

/* Controller reset + robust codec discovery */
static bool hda_controller_reset_and_start(pci_dev_t dev) {
	uint32_t bar0 = pci_read(dev, PCI_BAR0);
	if (pci_bar_type(bar0) != PCI_BAR_TYPE_MEMORY) {
		dprintf("hda: expected MMIO BAR0\n");
		return false;
	}
	hda.mmio = pci_mem_base(bar0);

	if (!pci_bus_master(dev)) {
		dprintf("hda: failed to set PCI bus master\n");
		return false;
	}

/* Hard reset sequence with WALLCLK-bounded settles (no OS timer needed) */
	hda_mmio_w32(HDA_REG_GCTL, 0);             /* CRST = 0 */
	hda_mmio_fence();                          /* flush posted write */

	/* hold reset low for ~0.5 ms */
	{
		uint32_t start = hda_mmio_r32(HDA_REG_WALCLK);
		uint32_t deadline = start + (uint32_t) (WALCLK_HZ / 2);
		while ((int32_t) (hda_mmio_r32(HDA_REG_WALCLK) - deadline) < 0) {
			__asm__ __volatile__("pause");
		}
	}

	/* deassert reset */
	hda_mmio_w32(HDA_REG_GCTL, 1); /* CRST = 1 */
	hda_mmio_fence();

	/* wait for CRST to latch (5 ms) */
	{
		uint32_t start = hda_mmio_r32(HDA_REG_WALCLK);
		uint32_t deadline = start + (5 * WALCLK_HZ);
		while ((hda_mmio_r32(HDA_REG_GCTL) & 1) == 0) {
			if ((int32_t) (hda_mmio_r32(HDA_REG_WALCLK) - deadline) >= 0) {
				break; /* don't hang forever if an odd platform is slow */
			}
			__asm__ __volatile__("pause");
		}
	}

	/* extra settle for codec enumeration (≤5 ms) */
	{
		uint32_t start = hda_mmio_r32(HDA_REG_WALCLK);
		uint32_t deadline = start + (5 * WALCLK_HZ);
		while ((int32_t) (hda_mmio_r32(HDA_REG_WALCLK) - deadline) < 0) {
			__asm__ __volatile__("pause");
		}
	}

	/* Clear wake/state/interrupts; don’t rely on STATESTS for presence */
	hda_mmio_w16(HDA_REG_STATESTS, hda_mmio_r16(HDA_REG_STATESTS));
	hda_mmio_w32(HDA_REG_INTSTS, hda_mmio_r32(HDA_REG_INTSTS));
	hda_mmio_w32(HDA_REG_INTCTL, 0);

	if (!hda_corb_rirb_init()) {
		return false;
	}

	/* Probe codecs 0..14 by sending a harmless GET_PARAMETER to NID 0 */
	bool found = false;
	for (uint8_t cad = 0; cad < 15; cad++) {
		uint32_t dummy = 0;
		if (hda_get_param(cad, 0x00, P_NODE_COUNT, &dummy)) {
			hda.cad = cad;
			found = true;
			break;
		}
	}
	if (!found) {
		dprintf("hda: no codecs responded to verbs\n");
		return false;
	}

	/* Walk to AFG and select an output path (DAC + output Pin) */
	nid_range_t wr;
	if (!hda_find_afg(hda.cad, &hda.afg_nid, &wr)) {
		dprintf("hda: no AFG found (cad=%u)\n", hda.cad);
		return false;
	}
	if (!hda_pick_output_path(hda.cad, wr, &hda.dac_nid, &hda.pin_nid)) {
		dprintf("hda: no suitable output path (cad=%u afg=0x%02x)\n", hda.cad, hda.afg_nid);
		hda_dump_topology(hda.cad, hda.afg_nid, wr);
		return false;
	}

	/* Power up AFG, DAC, Pin to D0 */
	hda_cmd(hda.cad, hda.afg_nid, V_SET_POWER_STATE, 0x00, NULL);
	hda_cmd(hda.cad, hda.dac_nid, V_SET_POWER_STATE, 0x00, NULL);
	hda_cmd(hda.cad, hda.pin_nid, V_SET_POWER_STATE, 0x00, NULL);

	/* Enable pin output and EAPD/BTL if present */
	hda_cmd(hda.cad, hda.pin_nid, V_SET_PIN_WCTRL, 0x40, NULL); /* OUT_EN */
	hda_cmd(hda.cad, hda.pin_nid, V_SET_EAPD_BTL, 0x02, NULL);  /* EAPD */

	/* NEW: unmute DAC & Pin output amplifiers */
	hda_unmute_codec_path();

	/* Use first output stream (index 0) and stream tag 1 */
	hda.out_sd_index = 0;
	hda.out_stream_tag = 1;

	/* Bind converter to our stream tag; channel 0 (left). Channels set later to 2. */
	hda_cmd(hda.cad, hda.dac_nid, V_SET_CONV_STREAMCH, (uint16_t) ((hda.out_stream_tag << 4) | 0x0), NULL);
	hda_cmd(hda.cad, hda.dac_nid, V_SET_CHANNEL_COUNT, 1, NULL); /* 2 channels (value = n-1) */

	return true;
}

/* Compute base address of our output SD register block */
static inline uint32_t hda_out_sd_base(void) {
	uint16_t gcap = hda_mmio_r16(HDA_REG_GCAP);
	uint8_t n_in = (gcap >> 8) & 0x0F;
	return HDA_SD_BASE + (uint32_t) (n_in + hda.out_sd_index) * HDA_SD_STRIDE;
}

/* Reset a stream descriptor cleanly */
static void hda_sd_reset(uint32_t sdbase) {
	/* stop */
	uint8_t ctl0 = hda_mmio_r8(sdbase + SD_CTL0);
	ctl0 &= ~SD_CTL_RUN;
	hda_mmio_w8(sdbase + SD_CTL0, ctl0);
	/* reset */
	ctl0 |= SD_CTL_SRST;
	hda_mmio_w8(sdbase + SD_CTL0, ctl0);
	for (int i = 0; i < 1000 && !(hda_mmio_r8(sdbase + SD_CTL0) & SD_CTL_SRST); i++) {
		tiny_delay();
	}
	ctl0 &= ~SD_CTL_SRST;
	hda_mmio_w8(sdbase + SD_CTL0, ctl0);
	for (int i = 0; i < 1000 && (hda_mmio_r8(sdbase + SD_CTL0) & SD_CTL_SRST); i++) {
		tiny_delay();
	}
	/* clear status */
	hda_mmio_w8(sdbase + SD_STS, hda_mmio_r8(sdbase + SD_STS));
}

/* Stream / BDL preparation */
static inline uint8_t ring_free32(uint8_t civ, int tail) {
	if (tail < 0) {
		return 32;
	}
	int free = (int) civ - tail - 1;
	while (free < 0) {
		free += 32;
	}
	return (uint8_t) free;
}

/* Prepare BDL + buffer, set SDn registers and codec format. */
static bool hda_stream_prepare(uint32_t frag_bytes, uint32_t rate_hz) {
	if (frag_bytes == 0) {
		frag_bytes = 4096;
	}
	/* HDA requires BDL entries multiple of 128 bytes, and BDL base 128-aligned */
	frag_bytes = (frag_bytes + 127) & ~127;
	if (frag_bytes & 3) {
		frag_bytes += (4 - (frag_bytes & 3)); /* stereo 16-bit alignment */
	}

	const uint32_t bdl_n = 32;
	const uint32_t total_bytes = bdl_n * frag_bytes;

	uint8_t *buf_raw = kmalloc_low(total_bytes + 128);
	hda_bdl_entry_t *bdl_raw = kmalloc_low(sizeof(hda_bdl_entry_t) * bdl_n + 128);
	if (!buf_raw || !bdl_raw) {
		dprintf("hda: OOM allocating BDL/audio\n");
		return false;
	}
	uintptr_t buf_al = ((uintptr_t) buf_raw + 127) & ~(uintptr_t) 127;
	uintptr_t bdl_al = ((uintptr_t) bdl_raw + 127) & ~(uintptr_t) 127;

	hda.buf = (uint8_t *) buf_al;
	hda.buf_phys = (uint32_t) buf_al;
	hda.buf = (uint8_t *) buf_al;
	hda.bdl = (hda_bdl_entry_t *) bdl_al;
	hda.bdl_phys = (uint32_t) bdl_al;
	hda.bdl_n = bdl_n;
	hda.frag_bytes = frag_bytes;
	hda.frag_frames = frag_bytes / 4; /* 4 bytes per frame (stereo S16) */
	hda.tail = -1;
	hda.started = false;
	hda.rate_hz = 44100;

	memset(hda.buf, 0, total_bytes);

	for (uint32_t i = 0; i < bdl_n; i++) {
		hda.bdl[i].addr = (uint64_t) (hda.buf_phys + i * hda.frag_bytes);
		hda.bdl[i].len = hda.frag_bytes;
		hda.bdl[i].flags = 0; /* IOC=0 (polled) */
	}

	/* Program stream descriptor */
	uint32_t sdbase = hda_out_sd_base();

	hda_sd_reset(sdbase);

	/* SDnBDP */
	hda_mmio_w32(sdbase + SD_BDPL, hda.bdl_phys);
	hda_mmio_w32(sdbase + SD_BDPU, 0);

	/* CBL = total bytes in cyclic buffer; LVI = last index (0..255), must be >= 1 */
	hda_mmio_w32(sdbase + SD_CBL, total_bytes);
	hda_mmio_w16(sdbase + SD_LVI, (uint16_t) (hda.bdl_n - 1));

	/* Program 44.1 kHz, 16-bit, 2-ch in both codec and SDnFMT */
	uint16_t fmt = (uint16_t) (SD_FMT_BASE_44K1 | SD_FMT_MULT_X1 | SD_FMT_DIV_1 | SD_FMT_BITS_16 | SD_FMT_CHANS(2));
	/* Converter format (verb uses same field encoding) */
	hda_cmd(hda.cad, hda.dac_nid, 0x002, fmt, NULL);
	/* Stream format register */
	hda_mmio_w16(sdbase + SD_FMT, fmt);

	/* SD_CTL: set stream tag (byte2[7:4]); leave RUN=0 for now */
	uint8_t ctl2 = (uint8_t) ((hda.out_stream_tag & 0x0F) << 4);
	hda_mmio_w8(sdbase + SD_CTL2, ctl2);

	return true;
}

/* Start the output engine */
static void hda_run_stream(void) {
	uint32_t sdbase = hda_out_sd_base();
	uint8_t ctl0 = hda_mmio_r8(sdbase + SD_CTL0);
	ctl0 |= SD_CTL_RUN;
	hda_mmio_w8(sdbase + SD_CTL0, ctl0);
}

/* CIV equivalent: read current buffer index from LPIB / bytes progressed → fragment index.
 * HDA doesn't expose CIV directly; we derive it.
 */
static inline uint8_t hda_current_index(void) {
	uint32_t sdbase = hda_out_sd_base();
	uint32_t lpib = hda_mmio_r32(sdbase + SD_LPIB); /* bytes since cycle start */
	return (uint8_t) ((lpib / hda.frag_bytes) & 31);
}

/* Queue one or more fragments into the ring */
static size_t hda_play_s16le(const int16_t *frames, size_t frame_count) {
	if (!hda.bdl || !hda.buf || hda.frag_frames == 0) {
		return 0;
	}
	uint8_t civ = hda_current_index();
	uint8_t free = ring_free32(civ, hda.tail);

	size_t frames_done = 0;
	while (free && frame_count) {
		uint8_t next = (uint8_t) ((hda.tail + 1) & 31);
		uint8_t *dst = hda.buf + (uint32_t) next * hda.frag_bytes;

		size_t chunk_frames = hda.frag_frames;
		if (chunk_frames > frame_count) {
			chunk_frames = frame_count;
		}

		size_t bytes_copy = chunk_frames * 4;
		memcpy(dst, frames + frames_done * 2, bytes_copy);
		if (bytes_copy < hda.frag_bytes) {
			memset(dst + bytes_copy, 0, hda.frag_bytes - bytes_copy);
		}

		hda.tail = next;

		/* If not started yet, the hardware will loop the programmed ring; nothing to poke here:
		   SD_LVI already set to bdl_n-1 and SD_CBL to full size. */

		if (!hda.started) {
			hda_run_stream();
			hda.started = true;
		}

		frames_done += chunk_frames;
		frame_count -= chunk_frames;
		free--;
	}
	return frames_done;
}

/* SW FIFO: TODO: Make this a reusable thing */
static bool q_ensure_cap(size_t extra_fr) {
	size_t live = s_q_len_fr - s_q_head_fr;
	size_t need = live + extra_fr;
	if (need <= s_q_cap_fr) {
		return true;
	}
	size_t new_cap = s_q_cap_fr ? s_q_cap_fr : 4096;
	while (new_cap < need) {
		new_cap <<= 1;
	}
	int16_t *new_buf = kmalloc(new_cap * 2 * sizeof(int16_t));
	if (!new_buf) {
		return false;
	}
	if (live) {
		memcpy(new_buf, s_q_pcm + (s_q_head_fr * 2), live * 2 * sizeof(int16_t));
	}
	if (s_q_pcm) {
		kfree(s_q_pcm);
	}
	s_q_pcm = new_buf;
	s_q_cap_fr = new_cap;
	s_q_len_fr = live;
	s_q_head_fr = 0;
	return true;
}

static size_t push_all_s16le(const int16_t *frames, size_t total_frames) {
	static int first = 1;
	if (first) {
		first = 0;
		dprintf("hda: first push: %lu frames\n", (unsigned long) total_frames);
	}
	if (!frames || total_frames == 0) {
		return 0;
	}
	if (!q_ensure_cap(total_frames)) {
		dprintf("hda: queue OOM (wanted %lu frames)\n", total_frames);
		return 0;
	}
	size_t live = s_q_len_fr - s_q_head_fr;
	memcpy(s_q_pcm + (live * 2), frames, total_frames * 2 * sizeof(int16_t));
	s_q_len_fr = live + total_frames;
	return total_frames;
}

/* Drain FIFO into HDA BDL (idle hook) */
static void hda_idle(void) {

	if (!hda.bdl || hda.frag_frames == 0 || s_paused) {
		return;
	}
	size_t pending = s_q_len_fr - s_q_head_fr;
	if (pending == 0) {
		return;
	}
	while (pending) {
		size_t chunk = (pending > hda.frag_frames) ? hda.frag_frames : pending;
		size_t pushed = hda_play_s16le(s_q_pcm + (s_q_head_fr * 2), chunk);
		if (pushed == 0) {
			break; /* ring full now */
		}
		s_q_head_fr += pushed;
		pending -= pushed;
	}
	if (s_q_head_fr == s_q_len_fr) {
		s_q_head_fr = 0;
		s_q_len_fr = 0;
	}

	/* If the stream halted due to underrun, restart cheaply */
	uint32_t sdbase = hda_out_sd_base();
	uint8_t ctl0 = hda_mmio_r8(sdbase + SD_CTL0);
	if (!(ctl0 & SD_CTL_RUN) && !s_paused) {
		hda_run_stream();
		hda.started = true;
	}
}

/* Pause / resume / stop */
static void hda_pause(void) {
	uint32_t sdbase = hda_out_sd_base();
	uint8_t ctl0 = hda_mmio_r8(sdbase + SD_CTL0);
	ctl0 &= ~SD_CTL_RUN;
	hda_mmio_w8(sdbase + SD_CTL0, ctl0);
	s_paused = true;
}

static void hda_resume(void) {
	(void) hda_out_sd_base();
	/* no special status to clear on SD; just RUN */
	hda_run_stream();
	s_paused = false;
	hda.started = true;
	hda_idle();
}

static void hda_stop_clear(void) {
	uint32_t sdbase = hda_out_sd_base();
	uint8_t ctl0 = hda_mmio_r8(sdbase + SD_CTL0);
	ctl0 &= ~SD_CTL_RUN;
	hda_mmio_w8(sdbase + SD_CTL0, ctl0);

	hda_sd_reset(sdbase);

	s_q_head_fr = 0;
	s_q_len_fr = 0;
	s_paused = false;
	hda.started = false;
}

/* Return total buffered audio (SW FIFO + HW DMA) in milliseconds.
 * Important: do NOT treat the entire cyclic CBL as 'buffered';
 * only count what we've actually queued.
 */
static uint32_t hda_buffered_ms(void) {
	if (!hda.rate_hz) {
		return 0;
	}

	/* Software FIFO frames */
	size_t sw_frames = s_q_len_fr - s_q_head_fr;

	/* If the stream hasn't been started yet, only SW FIFO counts.
	 * Reporting a huge HW buffer here prevents the mixer from pushing.
	 */
	if (!hda.started || !hda.bdl || hda.frag_frames == 0) {
		return (uint32_t) ((sw_frames * 1000) / hda.rate_hz);
	}

	/* Hardware side: estimate frames remaining from current play position to our tail.
	 * Derive current fragment index from LPIB, and remaining frames in the current fragment.
	 */
	uint32_t sdbase = hda_out_sd_base();
	uint32_t lpib = hda_mmio_r32(sdbase + SD_LPIB);
	uint32_t in_frag_bytes = lpib % hda.frag_bytes;
	uint32_t rem_bytes_in_frag = hda.frag_bytes - in_frag_bytes;
	size_t rem_frames_in_frag = rem_bytes_in_frag / 4;

	uint8_t civ = (uint8_t) ((lpib / hda.frag_bytes) & 31);

	size_t hw_frames = 0;

	if (hda.tail >= 0) {
		/* Number of whole fragments queued ahead of the current playing fragment */
		uint8_t tail = (uint8_t) hda.tail & 31;
		int ahead = (int) tail - (int) civ;
		while (ahead < 0) {
			ahead += (int) hda.bdl_n; /* modulo 32 */
		}

		/* If tail == civ, only the remainder of current fragment is pending.
		 * Otherwise, add the full frames for the 'ahead' complete fragments beyond the current one.
		 */
		if (ahead == 0) {
			hw_frames = rem_frames_in_frag;
		} else {
			/* current fragment remainder + complete fragments until tail */
			hw_frames = rem_frames_in_frag + (size_t) (ahead - 1) * (size_t) hda.frag_frames + (size_t) hda.frag_frames;
		}
	}

	size_t total_frames = sw_frames + hw_frames;
	return (uint32_t) ((total_frames * 1000) / hda.rate_hz);
}


static uint32_t hda_get_hz(void) {
	return 44100;
}

/* Module init / registration */
static audio_device_t *init_hda(void) {
	pci_dev_t dev = pci_get_device(0, 0, HDA_PCI_CLASSC);
	if (!dev.bits) {
		dprintf("hda: no devices (class 0x0403)\n");
		return NULL;
	}
	if (!hda_controller_reset_and_start(dev)) {
		dprintf("hda: controller bring-up failed\n");
		return NULL;
	}
	if (!hda_stream_prepare(4096, 44100)) {
		dprintf("hda: stream prepare failed\n");
		return NULL;
	}

	/* Finally, bind converter stream tag (again) and start later on first queue */
	hda_cmd(hda.cad, hda.dac_nid, V_SET_CONV_STREAMCH, (uint16_t) ((hda.out_stream_tag << 4) | 0), NULL);

	/* Register idle hook to drain the SW queue */
	proc_register_idle(hda_idle, IDLE_FOREGROUND, 1);

	/* Register with audio core */
	audio_device_t *device = kmalloc(sizeof(audio_device_t));
	make_unique_device_name("audio", device->name, MAX_AUDIO_DEVICE_NAME);
	device->opaque = &hda;
	device->next = NULL;
	device->play = push_all_s16le;
	device->frequency = hda_get_hz;
	device->pause = hda_pause;
	device->resume = hda_resume;
	device->stop = hda_stop_clear;
	device->queue_length = hda_buffered_ms;
	device->get_outputs = hda_list_output_names;
	device->select_output = hda_select_output_by_name;
	device->get_current_output = hda_get_current_output;

	return register_audio_device(device) ? device : NULL;
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("hda: module loaded\n");
	audio_device_t *dev;
	if (!(dev = init_hda())) {
		return false;
	}
	if (!mixer_init(dev, 50, 25, 64)) {
		dprintf("hda: mixer init failed\n");
		return false;
	}
	kprintf("hda: started\n");
	dprintf("hda: started (cad=%u, afg=0x%02x, dac=0x%02x, pin=0x%02x)\n", hda.cad, hda.afg_nid, hda.dac_nid, hda.pin_nid);
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	/* not unloadable yet */
	return false;
}
