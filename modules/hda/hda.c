#include <kernel.h>
#include <mmio.h>
#include "hda.h"

/* Driver state (single controller, single output stream) */
struct hda_dev {
	uint64_t mmio;          /* BAR0 MMIO base */
	uint8_t  codec;         /* usually 0 */
	uint8_t  afg;           /* audio function group nid */
	uint8_t  pin;           /* chosen output pin nid */
	uint8_t  dac;           /* converter nid feeding the pin */
	uint8_t  stream_id;     /* 1..15 */
	struct hda_bdl_entry *bdl;
	uint64_t bdl_phys;
	uint8_t *buf_a, *buf_b;
	uint64_t buf_a_phys, buf_b_phys;
	uint32_t half_bytes;
};
static struct hda_dev g;

/* --- MMIO helpers --- */
static inline uint32_t RD(uint32_t off) { return mmio_read32(g.mmio + off); }
static inline void     WR(uint32_t off, uint32_t v) { mmio_write32(g.mmio + off, v); }
static inline uint16_t RD16(uint32_t off) { return mmio_read16(g.mmio + off); }
static inline void     WR16(uint32_t off, uint16_t v) { mmio_write16(g.mmio + off, v); }
static inline void     WR8(uint32_t off, uint8_t v) { mmio_write8(g.mmio + off, v); }

static inline void delay_us(unsigned us) {
	/* Approximate microsecond delay using io_wait().
	   Each io_wait is typically ~1–4 µs on PC hardware. */
	for (unsigned i = 0; i < us; i++) {
		io_wait();
	}
}

/* --- Global reset --- */
/* Replace your current hda_global_reset() with this exact version */
static int hda_global_reset(void) {
	/* Clear CRST */
	uint32_t gctl = RD(HDA_GCTL);
	WR(HDA_GCTL, gctl & ~GCTL_CRST);
	(void)RD(HDA_GCTL);          /* post */

	/* Deassert reset (set CRST) */
	WR(HDA_GCTL, (gctl & ~GCTL_CRST) | GCTL_CRST);
	(void)RD(HDA_GCTL);          /* post */

	/* Wait for controller to report CRST=1 */
	for (int i = 0; i < 1000; i++) {
		if (RD(HDA_GCTL) & GCTL_CRST) {
			/* Spec requires ≥ ~0.5 ms before touching codecs after CRST=1.
			   Give it 1 ms to be safe. */
			delay_us(1000);
			return 0;
		}
		delay_us(10);
	}
	return -1; /* timed out leaving reset */
}

static volatile uint32_t *corb_ring = NULL;  /* 4 bytes per CORB entry */
static volatile uint32_t *rirb_ring = NULL;  /* 8 bytes per RIRB entry (two dwords) */

static int corb_rirb_init(void) {
	uint8_t *corb_raw = (uint8_t *)kmalloc_low(4096 + 128);
	uint8_t *rirb_raw = (uint8_t *)kmalloc_low(4096 + 128);
	if (!corb_raw || !rirb_raw) return -1;

	uintptr_t corb_al = ((uintptr_t)corb_raw + 127u) & ~(uintptr_t)127u;
	uintptr_t rirb_al = ((uintptr_t)rirb_raw + 127u) & ~(uintptr_t)127u;

	uint64_t corb_phys = (uint64_t)corb_al;  /* identity map → virt == phys */
	uint64_t rirb_phys = (uint64_t)rirb_al;

	corb_ring = (volatile uint32_t *)corb_al;
	rirb_ring = (volatile uint32_t *)rirb_al;

	/* Program physical bases */
	WR(HDA_CORBLBASE, (uint32_t)(corb_phys & 0xffffffffu));
	WR(HDA_CORBUBASE, (uint32_t)(corb_phys >> 32));
	WR(HDA_RIRBLBASE, (uint32_t)(rirb_phys & 0xffffffffu));
	WR(HDA_RIRBUBASE, (uint32_t)(rirb_phys >> 32));

	/* 256-entry rings */
	WR8(HDA_CORBSIZE, CORBSIZE_256);
	WR8(HDA_RIRBSIZE, RIRBSIZE_256);

	/* Reset pointers */
	WR16(HDA_CORBRP, 1u << 15);           /* reset → 0 */
	mmio_write8(g.mmio + HDA_CORBWP, 0);  /* CORBWP is 8-bit */
	WR16(HDA_RIRBWP, 1u << 15);           /* reset → 0 */
	WR16(HDA_RINTCNT, 1);

	/* Clear stale RIRB status */
	mmio_write8(g.mmio + HDA_RIRBSTS, 0x01); /* W1C: RINTFL */

	/* Enable DMA: CORB=run, RIRB=run+RINTCTL (we still poll RIRBSTS) */
	mmio_write8(g.mmio + HDA_CORBCTL, 0x02);
	mmio_write8(g.mmio + HDA_RIRBCTL, 0x03); /* ← this is the line you asked about */

	return 0;
}
/* Spec §3.4 Immediate Command I/F: ICOI=0x60, IRII=0x64, ICIS=0x68 */
static bool hda_send_verb_immediate(uint8_t cad, uint8_t nid,
				    uint16_t verb, uint16_t payload,
				    uint32_t *out_resp) {
	const uint32_t ICOI = 0x60, IRII = 0x64, ICIS = 0x68;
	const uint32_t ICB  = 1u << 0;  /* Immediate Command Busy */
	const uint32_t IRV  = 1u << 1;  /* Immediate Result Valid */

	/* Make command field: [31:28]=CAd, [27:20]=NID, [19:0]=verb+payload */
	uint32_t cmd = ((uint32_t)cad << 28) | ((uint32_t)nid << 20)
		       | (((uint32_t)verb & 0xFFF) << 8) | (payload & 0xFF);

	/* Ensure not busy */
	uint32_t t0 = time(NULL);
	while (mmio_read16(g.mmio + ICIS) & ICB) {
		if (time(NULL) != t0) { dprintf("hda: ICOI busy timeout\n"); return false; }
	}

	/* Clear prior IRV if any */
	if (mmio_read16(g.mmio + ICIS) & IRV) mmio_write16(g.mmio + ICIS, IRV);

	mmio_write32(g.mmio + ICOI, cmd);
	/* Kick: set ICB=1 */
	mmio_write16(g.mmio + ICIS, ICB);

	/* Wait for IRV */
	t0 = time(NULL);
	while ((mmio_read16(g.mmio + ICIS) & IRV) == 0) {
		if (time(NULL) != t0) { /* long delay => recover per spec */
			/* Clear ICB to abort and wait it to drop */
			mmio_write16(g.mmio + ICIS, 0);
			uint32_t t1 = time(NULL);
			while (mmio_read16(g.mmio + ICIS) & ICB) {
				if (time(NULL) != t1) break;
			}
			dprintf("hda: immediate verb timeout\n");
			return false;
		}
	}

	uint32_t resp = mmio_read32(g.mmio + IRII);
	/* Ack IRV */
	mmio_write16(g.mmio + ICIS, IRV);

	if (out_resp) *out_resp = resp;
	return true;
}


static int send_verb(uint8_t codec, uint8_t nid, uint16_t verb, uint16_t payload, uint32_t *out) {
	if (!corb_ring || !rirb_ring) return -1;

	/* queue one CORB entry then bump WP (8-bit, 256 entries) */
	uint8_t wp  = mmio_read8(g.mmio + HDA_CORBWP);
	uint8_t nwp = (uint8_t)(wp + 1u);
	corb_ring[nwp] = HDA_MAKE_VERB(codec, nid, verb, payload);
	mmio_write8(g.mmio + HDA_CORBWP, nwp);

	/* poll RIRB status bit (RIRBSTS bit0) for a new response */
	for (int i = 0; i < 1000; i++) {              /* ~1 s worst-case */
		uint8_t sts = mmio_read8(g.mmio + HDA_RIRBSTS);
		if (sts & 0x01u) {
			/* response available: read at index RIRBWP (two dwords per entry) */
			uint16_t wp2 = RD16(HDA_RIRBWP) & 0x00FFu; /* mask to 0..255 */
			const uint32_t *resp = (const uint32_t *)&rirb_ring[wp2 * 2u];
			*out = resp[0];
			mmio_write8(g.mmio + HDA_RIRBSTS, 0x01);   /* W1C: ack */
			return 0;
		}
		delay_us(1000);
	}

	dprintf("hda: verb timeout (c=%u nid=0x%x v=0x%x p=0x%x) CORBWP=%u RIRBWP=%u\n",
		codec, nid, verb, payload, mmio_read8(g.mmio + HDA_CORBWP), RD16(HDA_RIRBWP));
	return -1;
}

/* --- Crude discovery: pick an AFG, then a Pin with Speaker/HP/LineOut, assume DAC 0x02 --- */
static int pick_output_path(void) {
	g.codec = 0;

	uint32_t r = 0;
	if (send_verb(g.codec, 0x00, VERB_GET_PARAMETER, PARAM_NODE_COUNT, &r) != 0) {
		return -1;
	}
	uint8_t start = (uint8_t)((r >> 16) & 0xFFu);
	uint8_t count = (uint8_t)(r & 0xFFu);

	g.afg = 0xFFu;
	for (uint8_t n = start; n < (uint8_t)(start + count); n++) {
		if (send_verb(g.codec, n, VERB_GET_PARAMETER, PARAM_FUNC_GROUP_TYPE, &r) != 0) {
			continue;
		}
		if ((r & 0xFFu) == 0x01u) {
			g.afg = n;
			break;
		}
	}
	if (g.afg == 0xFFu) {
		return -1;
	}

	if (send_verb(g.codec, g.afg, VERB_GET_PARAMETER, PARAM_NODE_COUNT, &r) != 0) {
		return -1;
	}
	uint8_t wstart = (uint8_t)((r >> 16) & 0xFFu);
	uint8_t wcount = (uint8_t)(r & 0xFFu);

	g.pin = 0xFFu;
	for (uint8_t w = wstart; w < (uint8_t)(wstart + wcount); w++) {
		if (send_verb(g.codec, w, VERB_GET_PARAMETER, PARAM_AUDIO_WIDGET_CAP, &r) != 0) {
			continue;
		}
		uint8_t wtype = (uint8_t)((r >> 20) & 0x0Fu);
		if (wtype != WIDGET_TYPE_PIN) {
			continue;
		}
		if (send_verb(g.codec, w, VERB_GET_DEFAULT_CFG, 0, &r) != 0) {
			continue;
		}
		uint8_t defdev = (uint8_t)((r >> 20) & 0x0Fu);
		if (defdev == PIN_DEV_SPEAKER || defdev == PIN_DEV_HP_OUT || defdev == PIN_DEV_LINE_OUT) {
			g.pin = w;
			break;
		}
	}

	g.dac = 0x02u;            /* heuristic works on QEMU + many Realteks */
	if (g.pin == 0xFFu) {
		g.pin = 0x14u;        /* conservative fallback */
	}
	return 0;
}

static int power_and_unmute(void) {
	uint32_t r;

	g.pin = 0x1b; /* common line-out pin in QEMU hda-output */


	/* AFG → D0 first */
	if (send_verb(g.codec, g.afg, VERB_SET_POWER_STATE, 0x00u, &r) != 0) return -1;

	/* Then DAC and Pin */
	if (send_verb(g.codec, g.dac, VERB_SET_POWER_STATE, 0x00u, &r) != 0) return -1;
	if (send_verb(g.codec, g.pin, VERB_SET_POWER_STATE, 0x00u, &r) != 0) return -1;

	/* Enable output on the pin (bit 6) */
	if (send_verb(g.codec, g.pin, VERB_SET_PIN_WIDGET_CTRL, 0x40u, &r) != 0) return -1;

	/* EAPD = 1 (ok if ignored) */
	(void)send_verb(g.codec, g.pin, VERB_SET_EAPD_BTL, 0x02u, &r);

	/* OUTPUT amp, both channels, unmute, 0 dB: 1<<15 | 3<<12 | 1<<11 | mute=0 | gain=0 */
	const uint16_t AMP_OUT_BOTH_UNMUTE = 0xB800;
	(void)send_verb(g.codec, g.dac, VERB_SET_AMP_GAIN_MUTE, AMP_OUT_BOTH_UNMUTE, &r);
	(void)send_verb(g.codec, g.pin, VERB_SET_AMP_GAIN_MUTE, AMP_OUT_BOTH_UNMUTE, &r);

	return 0;
}

static uint16_t fmt_48k_s16le_stereo(void) {
	/* SDnFMT: [14:11] rate (48 kHz => 0), [7:4] bits code (16-bit => 1), [3:0] channels-1 (2ch => 1) */
	const uint16_t SR_48K = 0u << 11;
	const uint16_t BITS16 = 0x1u << 4;
	const uint16_t CH2    = 0x1u;
	return (uint16_t)(SR_48K | BITS16 | CH2); /* 0x0011 */
}

static int stream0_setup(void) {
	g.stream_id  = 1;
	g.half_bytes = 16384u; /* 4096 frames * 4 bytes/frame ≈ 85 ms */

	/* Allocate and 128B-align BDL and halves (<4 GiB phys) */
	uint8_t *bdl_raw = (uint8_t *)kmalloc_low(sizeof(struct hda_bdl_entry) * 2u + 128u);
	if (!bdl_raw) return -1;
	uintptr_t bdl_al = ((uintptr_t)bdl_raw + 127u) & ~(uintptr_t)127u;
	g.bdl      = (struct hda_bdl_entry *)bdl_al;
	g.bdl_phys = (uint64_t)bdl_al;

	uint8_t *a_raw = (uint8_t *)kmalloc_low(g.half_bytes + 128u);
	uint8_t *b_raw = (uint8_t *)kmalloc_low(g.half_bytes + 128u);
	if (!a_raw || !b_raw) return -1;
	uintptr_t a_al = ((uintptr_t)a_raw + 127u) & ~(uintptr_t)127u;
	uintptr_t b_al = ((uintptr_t)b_raw + 127u) & ~(uintptr_t)127u;
	g.buf_a = (uint8_t *)a_al; g.buf_a_phys = (uint64_t)a_al;
	g.buf_b = (uint8_t *)b_al; g.buf_b_phys = (uint64_t)b_al;
	memset(g.buf_a, 0, g.half_bytes);
	memset(g.buf_b, 0, g.half_bytes);

	/* Reset SD0 */
	uint32_t ctl0 = RD(HDA_SD0CTL);
	WR(HDA_SD0CTL, ctl0 | SDCTL_SRST);
	delay_us(10);
	WR(HDA_SD0CTL, ctl0 & ~SDCTL_SRST);

	/* Program BDL (BDPL/BDPU at 0x98/0x9C) and buffer sizes */
	g.bdl[0].addr_lo = (uint32_t)(g.buf_a_phys & 0xffffffffu);
	g.bdl[0].addr_hi = (uint32_t)(g.buf_a_phys >> 32);
	g.bdl[0].length  = g.half_bytes;
	g.bdl[0].flags   = 0;

	g.bdl[1].addr_lo = (uint32_t)(g.buf_b_phys & 0xffffffffu);
	g.bdl[1].addr_hi = (uint32_t)(g.buf_b_phys >> 32);
	g.bdl[1].length  = g.half_bytes;
	g.bdl[1].flags   = 0;

	WR(HDA_SD0BDPL, (uint32_t)(g.bdl_phys & 0xffffffffu));
	WR(HDA_SD0BDPU, (uint32_t)(g.bdl_phys >> 32));
	WR(HDA_SD0CBL,  g.half_bytes * 2u);
	WR16(HDA_SD0LVI, 1u); /* two entries: 0..1 */

	/* Set PCM format at 0x92 */
	uint16_t fmt = fmt_48k_s16le_stereo();
	WR16(HDA_SD0FMT, fmt);

	/* Tell the converter (stream/tag + format) */
	uint32_t r;
	if (send_verb(g.codec, g.dac, VERB_SET_CONV_STREAM_CHAN, (uint16_t)(g.stream_id << 4), &r) != 0) return -1;
	if (send_verb(g.codec, g.dac, VERB_SET_CONV_FMT, fmt, &r) != 0) return -1;

	/* Write Stream ID into SD0CTL bits [23:20] to match converter tag */
	uint32_t ctl = RD(HDA_SD0CTL);
	ctl &= ~(0xFu << 20);
	ctl |= ((uint32_t)(g.stream_id & 0x0Fu)) << 20;
	WR(HDA_SD0CTL, ctl);

	/* Ack stale status and RUN */
	WR8(HDA_SD0STS, RD(HDA_SD0STS));  /* write back to clear W1C bits */
	WR(HDA_SD0CTL, ctl | SDCTL_RUN);
	return 0;
}

/* ---- public entry points ---- */

bool hda_start(pci_dev_t dev) {
	uint32_t bar0 = pci_read(dev, PCI_BAR0);
	if (pci_bar_type(bar0) != PCI_BAR_TYPE_MEMORY) {
		dprintf("hda: BAR0 not MMIO\n");
		return false;
	}
	g.mmio = pci_mem_base(bar0);
	kprintf("hda: mmio base %lx\n", (uint64_t)g.mmio);
	pci_bus_master(dev);

	/* Global reset with ≥1 ms guard after CRST=1 */
	if (hda_global_reset() != 0) {
		dprintf("hda: global reset failed\n");
		return false;
	}

	/* === YOUR QUESTION: the immediate probe goes RIGHT HERE === */
	{
		uint32_t resp = 0;
		if (!hda_send_verb_immediate(0 /* CAd0 */, 0 /* NID0 */,
					     0xF00 /* GET_PARAMETER */,
					     0x00  /* VENDOR_ID param */,
					     &resp)) {
			dprintf("hda: immediate GET_PARAMETER(VENDOR_ID) failed\n");
			return false;
		}
		dprintf("hda: codec0 root vendor/device = 0x%08x\n", resp);
	}
	/* === end of immediate probe === */

	if (corb_rirb_init() != 0) {
		dprintf("hda: corb/rirb init failed\n");
		return false;
	}

	if (pick_output_path() != 0) {
		g.codec = 0; g.afg = 1; g.pin = 0x14u; g.dac = 0x02u;
		dprintf("hda: using fallback pin 0x14 → dac 0x02\n");
	}

	if (power_and_unmute() != 0) {
		dprintf("hda: power/unmute failed\n");
		return false;
	}
	if (stream0_setup() != 0) {
		dprintf("hda: stream setup failed\n");
		return false;
	}

	kprintf("hda: ready (codec %u, afg %u, pin 0x%u, dac 0x%u)\n", (uint32_t)g.codec, (uint32_t)g.afg, (uint32_t)g.pin, (uint32_t)g.dac);
	return true;
}

/* Probe by class code 0x0403 and start, mirroring your init_ahci() pattern. */
bool init_hda(void) {
	pci_dev_t hda_device = pci_get_device(0, 0, PCI_CLASSCODE_HDA);
	if (!hda_device.bits) {
		dprintf("hda: no controllers found\n");
		return false;
	}

	uint64_t bar0 = pci_read(hda_device, PCI_BAR0);
	if (pci_bar_type(bar0) != PCI_BAR_TYPE_MEMORY) {
		dprintf("hda: controller BAR0 is not MMIO\n");
		return false;
	}

	if (!hda_start(hda_device)) {
		dprintf("hda: start failed\n");
		return false;
	}

	return true;
}


/* Blocking writer: copies whole frames (S16LE stereo) into the half that just finished. Underrun → silence. */
size_t hda_write_frames_48k_s16le(const int16_t *frames, size_t frame_count) {
	const uint32_t bpf = 4u; /* bytes per frame */
	uint8_t last_half = 1;   /* force first refill on A */
	size_t done = 0;

	while (done < frame_count) {
		uint32_t lpib = RD(HDA_SD0LPIB);
		uint32_t pos  = lpib % (g.half_bytes * 2u);
		uint8_t  now  = (pos < g.half_bytes) ? 0u : 1u;

		if (now != last_half) {
			uint8_t *dst = (last_half == 0u) ? g.buf_a : g.buf_b;
			uint32_t frames_half = g.half_bytes / bpf;
			uint32_t todo = frames_half;
			if (todo > (uint32_t)(frame_count - done)) {
				todo = (uint32_t)(frame_count - done);
			}
			memcpy(dst, frames + done * 2u, todo * bpf);
			if (todo < frames_half) {
				memset(dst + todo * bpf, 0, (frames_half - todo) * bpf);
			}
			done += todo;
			last_half = now;
		} else {
			delay_us(200);
		}
	}
	return done;
}

/* Non-blocking: write a 440 Hz square wave into both halves.
   Assumes stream0_setup() has already started SD0 (RUN set). */
/* Drop-in: replaces your existing hda_test_tone(void)
   - Fills both halves with a simple 440 Hz square (48 kHz, S16LE stereo)
   - If SD0 isn't running, it (re)arms the stream and starts it
*/
void hda_test_tone(void) {
	if (g.half_bytes == 0 || !g.bdl || !g.buf_a || !g.buf_b) {
		dprintf("hda: test_tone skipped (stream buffers not initialised)\n");
		return;
	}

	/* --- 1) Fill the two halves with a square wave --- */
	const uint32_t rate        = 48000;
	const uint32_t freq        = 440;
	const uint32_t period      = (freq ? (rate / freq) : 1);     /* ~109 */
	const uint32_t halfperiod  = (period ? (period / 2) : 1);
	const int16_t  amp         = 3000;
	const uint32_t frames_half = g.half_bytes / 4;               /* 2ch * 2 bytes */

	int16_t *pa = (int16_t *)g.buf_a;
	int16_t *pb = (int16_t *)g.buf_b;
	for (uint32_t i = 0; i < frames_half; i++) {
		int16_t s = ((i % period) < halfperiod) ? amp : (int16_t)-amp;
		pa[(i<<1)    ] = s;  /* L */
		pa[(i<<1) + 1] = s;  /* R */
		pb[(i<<1)    ] = s;
		pb[(i<<1) + 1] = s;
	}

	/* --- 2) If not already RUN, (re)arm SD0 and start it --- */
	if ((mmio_read8(g.mmio + HDA_SD0CTL) & SDCTL_RUN) == 0) {
		/* Reset SD0 cleanly */
		uint8_t ctl8 = mmio_read8(g.mmio + HDA_SD0CTL);
		mmio_write8(g.mmio + HDA_SD0CTL, ctl8 | SDCTL_SRST);
		delay_us(10);
		mmio_write8(g.mmio + HDA_SD0CTL, ctl8 & (uint8_t)~SDCTL_SRST);

		/* Program BDL (already built in init), sizes and format */
		WR(HDA_SD0BDPL, (uint32_t)(g.bdl_phys & 0xffffffffu));
		WR(HDA_SD0BDPU, (uint32_t)(g.bdl_phys >> 32));
		WR(HDA_SD0CBL,  g.half_bytes * 2u);
		WR16(HDA_SD0LVI, 1u);

		/* 48 kHz / 16-bit / 2-ch */
		const uint16_t fmt = 0x0011;
		WR16(HDA_SD0FMT, fmt);

		/* Bind converter to stream tag (ID=1) and format */
		uint32_t r;
		(void)send_verb(g.codec, g.dac, VERB_SET_CONV_STREAM_CHAN, (uint16_t)(g.stream_id << 4), &r);
		(void)send_verb(g.codec, g.dac, VERB_SET_CONV_FMT, fmt, &r);

		/* Write Stream ID into SD0CTL[23:20] */
		uint32_t ctl32 = RD(HDA_SD0CTL);
		ctl32 &= ~(0xFu << 20);
		ctl32 |= ((uint32_t)(g.stream_id & 0x0Fu)) << 20;
		WR(HDA_SD0CTL, ctl32);

		/* Ack stale status and RUN */
		mmio_write8(g.mmio + HDA_SD0STS, 0xFF);
		WR(HDA_SD0CTL, ctl32 | SDCTL_RUN);
	}

	/* Optional: tiny visibility that DMA is alive */
	uint32_t lp0 = RD(HDA_SD0LPIB);
	for (int i = 0; i < 50; i++) delay_us(1000);  /* ~50 ms */
	uint32_t lp1 = RD(HDA_SD0LPIB);
	dprintf("hda: test_tone loaded; LPIB %u -> %u, RUN=%d\n",
		lp0, lp1, !!(mmio_read8(g.mmio + HDA_SD0CTL) & SDCTL_RUN));
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("hda: module loaded\n");
	if (!init_hda()) {
		return false;
	}
	dprintf("hda: init complete\n");
	hda_test_tone();
	dprintf("test tone emitted\n");
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	return false;
}
