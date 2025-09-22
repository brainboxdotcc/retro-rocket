/**
 * @file modules/hda/hda.c
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 *
 * Intel High Definition Audio (Azalia) output driver
 * Contract matches AC'97 module: async SW FIFO push, idle drain into HW ring.
 *
 * References:
 *  - Intel High Definition Audio Specification Rev. 1.0a (§3.3.* stream regs, §3.4 ICOI/ICII/ICIS,
 *    CORB/RIRB, SDnFMT encoding, BDL alignment) and widget verbs. (intel.com PDF)
 *  - OSDev HDA overview for register map & verb list (sanity cross-check).
 */
#include <kernel.h>
#include "audio.h"

#define HDA_PCI_CLASSC 0x0403

/* ---------- Controller MMIO registers (offsets from BAR0) ---------- */
#define HDA_REG_GCAP      0x00 /* Global Capabilities */
#define HDA_REG_VMIN      0x02
#define HDA_REG_VMAJ      0x03
#define HDA_REG_GCTL      0x08 /* bit0=CRST */
#define HDA_REG_WAKEEN    0x0C
#define HDA_REG_STATESTS  0x0E /* codec presence bits 0..2 */
#define HDA_REG_INTCTL    0x20
#define HDA_REG_INTSTS    0x24
#define HDA_REG_WALCLK    0x30
#define HDA_REG_SSYNC     0x34

/* CORB / RIRB */
#define HDA_REG_CORBLBASE 0x40
#define HDA_REG_CORBUBASE 0x44
#define HDA_REG_CORBWP    0x48
#define HDA_REG_CORBRP    0x4A
#define  HDA_CORBRP_RST   0x8000
#define HDA_REG_CORBCTL   0x4C
#define  HDA_CORBCTL_MEIE 0x01
#define  HDA_CORBCTL_DMAE 0x02
#define HDA_REG_CORBSTS   0x4D
#define HDA_REG_CORBSIZE  0x4E /* bits[1:0]=00:2,01:16,10:256 entries; feature bits [6:4] */
#define HDA_REG_RIRBLBASE 0x50
#define HDA_REG_RIRBUBASE 0x54
#define HDA_REG_RIRBWP    0x58
#define  HDA_RIRBWP_RST   0x8000
#define HDA_REG_RINTCNT   0x5A
#define HDA_REG_RIRBCTL   0x5C
#define  HDA_RIRBCTL_INT_ON_OVERRUN 0x01
#define  HDA_RIRBCTL_DMAE 0x02
#define  HDA_RIRBCTL_IRQE 0x04
#define HDA_REG_RIRBSTS   0x5D
#define HDA_REG_RIRBSIZE  0x5E

/* DMA position buffer (unused here, but handy for debugging latency) */
#define HDA_REG_DPLBASE   0x70
#define HDA_REG_DPUBASE   0x74

/* Stream descriptor block: layout is 0x80 + n*0x20; order: input, output, bidi */
#define HDA_SD_BASE       0x80
#define HDA_SD_STRIDE     0x20

/* Offsets inside SDn */
#define SD_CTL0    0x00 /* byte0: RUN(1), SRST(0); byte2: stream tag (7:4) + traf prio */
#define SD_CTL2    0x02
#define SD_STS     0x03
#define SD_LPIB    0x04
#define SD_CBL     0x08
#define SD_LVI     0x0C
#define SD_FMT     0x12
#define SD_BDPL    0x18
#define SD_BDPU    0x1C

/* SD_CTL0 bits */
#define SD_CTL_RUN  (1u << 1)
#define SD_CTL_SRST (1u << 0)

/* BDL entry (HDA): 16 bytes */
typedef struct {
	uint64_t addr;
	uint32_t len;
	uint32_t flags; /* bit0 = IOC */
} __attribute__((packed)) hda_bdl_entry_t;

/* Verb construction: [31:28] cad, [27:20] nid, [19:8] verb, [7:0] payload */
#define HDA_MAKE_VERB(cad, nid, verb, payload) \
    ( ((uint32_t)(cad) & 0xF) << 28 | ((uint32_t)(nid) & 0x7F) << 20 | ((uint32_t)(verb) & 0xFFF) << 8 | ((uint32_t)(payload) & 0xFF) )

/* Useful verbs (see spec / OSDev table) */
#define V_GET_PARAM         0xF00
#define V_GET_CONN_LIST     0xF02
#define V_SET_POWER_STATE   0x705 /* data: 0=D0 */
#define V_SET_CONV_STREAMCH 0x706 /* data: stream tag (7:4), channel (3:0) */
#define V_SET_PIN_WCTRL     0x707 /* data bits: OUT_EN=bit6, HP_EN=bit7, IN_EN=bit5 */
#define V_GET_PIN_SENSE     0xF09
#define V_SET_EAPD_BTL      0x70C /* data bit1=EAPD */
#define V_GET_DEF_CFG       0xF1C
#define V_SET_CHANNEL_COUNT 0x72D /* data: channels-1 */
#define V_SET_AMP_GAIN_MUTE 0x003 /* 16-bit payload via immediate data */

/* GET_PARAM indices */
#define P_NODE_COUNT        0x04 /* high byte: start node; low byte: count (AFG or func grp) */
#define P_FUNC_GRP_TYPE     0x05 /* expect 1 = AFG */
#define P_AW_CAPS           0x09 /* Audio Widget Capabilities (type in bits 23:20) */
#define P_PIN_CAPS          0x0C

/* Widget types (from P_AW_CAPS bits 23:20) */
#define WTYPE_AUDIO_OUT     0x01
#define WTYPE_AUDIO_IN      0x02
#define WTYPE_MIXER         0x02 /* varies by doc, but we only care OUT and PIN */
#define WTYPE_PIN_COMPLEX   0x04

/* Pin caps bits */
#define PINCAP_OUT          (1u << 4)

/* Default config decode (spec): 31:30 connectivity, 29:24 location, 23:20 default device, 19:16 conn type */
#define DEFDEV_SPEAKER      0x01
#define DEFDEV_LINEOUT      0x02
#define DEFDEV_HP_OUT       0x0F

/* SDnFMT fields (spec §3.3.41) */
#define SD_FMT_BASE_48K     (0u << 14)
#define SD_FMT_BASE_44K1    (1u << 14)
#define SD_FMT_MULT_X1      (0u << 11)
#define SD_FMT_MULT_X2      (1u << 11)
#define SD_FMT_DIV_1        (0u << 8)
#define SD_FMT_DIV_2        (1u << 8)
/* Bits per sample */
#define SD_FMT_BITS_16      (1u << 4)
/* Channels: value = (channels-1) */
#define SD_FMT_CHANS(n)     ((uint16_t)(((n) - 1u) & 0x0F))

/* ---------- Driver state ---------- */
typedef struct {
	uint32_t mmio;              /* BAR0 MMIO base (mapped) */
	/* CORB/RIRB rings (phys + virt) */
	uint64_t *corb;
	uint64_t *rirb;
	uint32_t corb_phys;
	uint32_t rirb_phys;
	uint8_t  corb_entries;      /* 256 */
	uint8_t  rirb_entries;      /* 256 */

	/* Chosen codec/function/wid path */
	uint8_t  cad;               /* codec address (0..15) */
	uint8_t  afg_nid;           /* audio function group node id */
	uint8_t  dac_nid;           /* output converter (DAC) NID */
	uint8_t  pin_nid;           /* output pin NID */

	/* Single output stream descriptor we use (first HW output stream) */
	uint8_t  out_sd_index;      /* 0-based among output SDs */
	uint8_t  out_stream_tag;    /* 1..15; we use 1 */

	/* BDL / audio ring */
	hda_bdl_entry_t *bdl;
	uint32_t         bdl_phys;
	uint8_t         *buf;
	uint32_t         buf_phys;
	uint32_t         bdl_n;         /* 32 */
	uint32_t         frag_bytes;    /* fragment size in bytes */
	uint32_t         frag_frames;   /* frames per fragment (S16LE stereo => 4 bytes/frame) */
	int              tail;          /* last valid index in the ring (−1 if empty) */
	bool             started;
	uint32_t         rate_hz;       /* 44100 */

} hda_dev_t;

static hda_dev_t hda;
static int16_t *s_q_pcm = NULL;
static size_t s_q_cap_fr = 0, s_q_len_fr = 0, s_q_head_fr = 0;
static bool s_paused = false;

/* ---------- Small MMIO helpers ---------- */
static inline uint8_t  mmio_r8(uint32_t off)  { return *((volatile uint8_t *)(uintptr_t)(hda.mmio + off)); }
static inline uint16_t mmio_r16(uint32_t off) { return *((volatile uint16_t*)(uintptr_t)(hda.mmio + off)); }
static inline uint32_t mmio_r32(uint32_t off) { return *((volatile uint32_t*)(uintptr_t)(hda.mmio + off)); }
static inline void mmio_w8 (uint32_t off, uint8_t  v) { *((volatile uint8_t *)(uintptr_t)(hda.mmio + off))  = v; }
static inline void mmio_w16(uint32_t off, uint16_t v) { *((volatile uint16_t*)(uintptr_t)(hda.mmio + off))  = v; }
static inline void mmio_w32(uint32_t off, uint32_t v) { *((volatile uint32_t*)(uintptr_t)(hda.mmio + off))  = v; }

/* Delay helper: short micro-spins via io_wait() */
static inline void tiny_delay(void) {
	for (int i = 0; i < 64; i++) {
		io_wait();
	}
}

/* ---------- CORB/RIRB utilities (polled) ---------- */
static bool hda_corb_rirb_init(void) {
	/* Stop DMA and reset pointers */
	mmio_w8(HDA_REG_CORBCTL, 0);
	mmio_w8(HDA_REG_RIRBCTL, 0);

	/* Program sizes to 256 entries (if supported) */
	uint8_t corbsz = mmio_r8(HDA_REG_CORBSIZE);
	uint8_t rirbsz = mmio_r8(HDA_REG_RIRBSIZE);
	if (!(corbsz & (1u << 6)) || !(rirbsz & (1u << 6))) {
		dprintf("hda: controller does not advertise 256-entry CORB/RIRB\n");
		return false;
	}
	mmio_w8(HDA_REG_CORBSIZE, (mmio_r8(HDA_REG_CORBSIZE) & ~0x03u) | 0x02u);
	mmio_w8(HDA_REG_RIRBSIZE, (mmio_r8(HDA_REG_RIRBSIZE) & ~0x03u) | 0x02u);

	/* Allocate 256-entry rings, 1 KiB aligned (controller requires >=1 KiB) */
	size_t corb_bytes = 256 * sizeof(uint64_t);
	size_t rirb_bytes = 256 * sizeof(uint64_t);
	void *corb_raw = kmalloc_low(corb_bytes + 1024);
	void *rirb_raw = kmalloc_low(rirb_bytes + 1024);
	if (!corb_raw || !rirb_raw) {
		dprintf("hda: OOM allocating CORB/RIRB\n");
		return false;
	}
	uintptr_t corb_al = ((uintptr_t)corb_raw + 1023u) & ~(uintptr_t)1023u;
	uintptr_t rirb_al = ((uintptr_t)rirb_raw + 1023u) & ~(uintptr_t)1023u;

	hda.corb = (uint64_t *)corb_al;
	hda.rirb = (uint64_t *)rirb_al;
	hda.corb_phys = (uint32_t)corb_al;
	hda.rirb_phys = (uint32_t)rirb_al;
	hda.corb_entries = 256;
	hda.rirb_entries = 256;

	memset(hda.corb, 0, corb_bytes);
	memset(hda.rirb, 0, rirb_bytes);

	/* Program base addresses; reset read pointers */
	mmio_w32(HDA_REG_CORBLBASE, hda.corb_phys);
	mmio_w32(HDA_REG_CORBUBASE, 0);
	mmio_w16(HDA_REG_CORBRP, HDA_CORBRP_RST);
	while (!(mmio_r16(HDA_REG_CORBRP) & HDA_CORBRP_RST)) { tiny_delay(); }
	mmio_w16(HDA_REG_CORBRP, 0); /* clear reset */

	mmio_w32(HDA_REG_RIRBLBASE, hda.rirb_phys);
	mmio_w32(HDA_REG_RIRBUBASE, 0);
	mmio_w16(HDA_REG_RIRBWP, HDA_RIRBWP_RST);
	/* spec: write of RIRBWP reset always reads as 0, proceed */

	/* Response interrupt after every entry */
	mmio_w16(HDA_REG_RINTCNT, 1);

	/* Enable rings */
	mmio_w8(HDA_REG_CORBCTL, HDA_CORBCTL_DMAE);
	mmio_w8(HDA_REG_RIRBCTL, HDA_RIRBCTL_DMAE);

	return true;
}

/* Issue a single verb via CORB, wait for response in RIRB (polling). */
static bool hda_cmd(uint8_t cad, uint8_t nid, uint16_t verb, uint16_t payload, uint32_t *resp_out) {
	uint32_t wp = mmio_r16(HDA_REG_CORBWP) & 0xFFu;
	uint32_t next = (wp + 1u) & 0xFFu;
	uint32_t old_rirb_wp = mmio_r16(HDA_REG_RIRBWP) & 0xFFu;

	uint32_t v = HDA_MAKE_VERB(cad, nid, verb, payload);
	hda.corb[next] = (uint64_t)v;

	/* Publish new write pointer */
	mmio_w16(HDA_REG_CORBWP, (uint16_t)next);

	/* Poll RIRB for one new entry */
	for (int i = 0; i < 10000; i++) {
		uint32_t nw = mmio_r16(HDA_REG_RIRBWP) & 0xFFu;
		if (nw != old_rirb_wp) {
			uint64_t r = hda.rirb[nw];
			if (resp_out) {
				*resp_out = (uint32_t)(r & 0xFFFFFFFFu);
			}
			return true;
		}
		tiny_delay();
	}
	dprintf("hda: verb 0x%03x to cad %u nid %u timed out\n", verb, cad, nid);
	return false;
}

/* GET_PARAMETER helper returning 32-bit value */
static inline bool hda_get_param(uint8_t cad, uint8_t nid, uint8_t param, uint32_t *out) {
	return hda_cmd(cad, nid, V_GET_PARAM, param, out);
}

/* ---------- Codec topology discovery (pick DAC + output Pin) ---------- */
typedef struct { uint8_t start_nid; uint8_t count; } nid_range_t;

static bool hda_find_afg(uint8_t cad, uint8_t *afg_nid, nid_range_t *widgets) {
	uint32_t func_range;
	if (!hda_get_param(cad, 0x00, P_NODE_COUNT, &func_range)) {
		return false;
	}
	uint8_t fn_start = (uint8_t)((func_range >> 16) & 0xFFu);
	uint8_t fn_count = (uint8_t)(func_range & 0xFFu);
	for (uint8_t i = 0; i < fn_count; i++) {
		uint8_t fn = fn_start + i;
		uint32_t ftype;
		if (!hda_get_param(cad, fn, P_FUNC_GRP_TYPE, &ftype)) {
			continue;
		}
		if ((ftype & 0xFFu) == 0x01u) { /* AFG */
			uint32_t w = 0;
			if (!hda_get_param(cad, fn, P_NODE_COUNT, &w)) {
				continue;
			}
			widgets->start_nid = (uint8_t)((w >> 16) & 0xFFu);
			widgets->count     = (uint8_t)(w & 0xFFu);
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
		*type_out = (uint8_t)((caps >> 20) & 0x0Fu);
	}
	return true;
}

/* choose an output DAC and an output pin that can be wired to it (directly or via selector).
   heuristic: pick first Pin with PINCAP_OUT and default device speaker/lineout,
   then scan its connection list to find a DAC. */
static bool hda_pick_output_path(uint8_t cad, nid_range_t wr, uint8_t *out_dac, uint8_t *out_pin) {
	/* enumerate pins of interest */
	for (uint8_t n = 0; n < wr.count; n++) {
		uint8_t nid = wr.start_nid + n;
		uint8_t wtype; uint32_t wcaps;
		if (!hda_widget_type(cad, nid, &wtype, &wcaps)) {
			continue;
		}
		if (wtype != WTYPE_PIN_COMPLEX) {
			continue;
		}
		uint32_t pincaps;
		if (!hda_get_param(cad, nid, P_PIN_CAPS, &pincaps)) {
			continue;
		}
		if (!(pincaps & PINCAP_OUT)) {
			continue;
		}
		/* default cfg filter: prefer speaker/lineout/headphone */
		uint32_t defcfg;
		if (!hda_cmd(cad, nid, V_GET_DEF_CFG, 0, &defcfg)) {
			continue;
		}
		uint8_t dev = (uint8_t)((defcfg >> 20) & 0x0Fu);
		if (!(dev == DEFDEV_SPEAKER || dev == DEFDEV_LINEOUT || dev == DEFDEV_HP_OUT)) {
			continue;
		}

		/* inspect pin's connection list length, then entries, searching for an Audio Output */
		uint32_t connlen;
		if (!hda_get_param(cad, nid, 0x0E /* connection list length */, &connlen)) {
			continue;
		}
		uint32_t entries = connlen & 0x7Fu;
		for (uint32_t idx = 0; idx < entries; idx++) {
			uint32_t conn;
			if (!hda_cmd(cad, nid, V_GET_CONN_LIST, (uint16_t)idx, &conn)) {
				break;
			}
			uint8_t peer = (uint8_t)(conn & 0xFFu);
			uint8_t ptype; uint32_t pcaps;
			if (!hda_widget_type(cad, peer, &ptype, &pcaps)) {
				continue;
			}
			if (ptype == WTYPE_AUDIO_OUT) {
				*out_pin = nid;
				*out_dac = peer;
				return true;
			}
		}
	}
	/* fallback: first DAC and first output Pin if the above failed */
	uint8_t first_dac = 0, first_pin = 0;
	for (uint8_t n = 0; n < wr.count; n++) {
		uint8_t nid = wr.start_nid + n;
		uint8_t t; uint32_t c;
		if (!hda_widget_type(cad, nid, &t, &c)) {
			continue;
		}
		if (!first_dac && t == WTYPE_AUDIO_OUT) {
			first_dac = nid;
		}
		if (!first_pin && t == WTYPE_PIN_COMPLEX) {
			uint32_t pc;
			if (hda_get_param(cad, nid, P_PIN_CAPS, &pc) && (pc & PINCAP_OUT)) {
				first_pin = nid;
			}
		}
	}
	if (first_dac && first_pin) {
		*out_dac = first_dac;
		*out_pin = first_pin;
		return true;
	}
	return false;
}

/* ---------- Controller reset + basic bring-up ---------- */
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

	/* Put controller in reset then take out of reset */
	mmio_w32(HDA_REG_GCTL, 0);
	for (int i = 0; i < 1000 && (mmio_r32(HDA_REG_GCTL) & 1u); i++) {
		tiny_delay();
	}
	mmio_w32(HDA_REG_GCTL, 1); /* CRST=1 */
	for (int i = 0; i < 1000 && !(mmio_r32(HDA_REG_GCTL) & 1u); i++) {
		tiny_delay();
	}

	/* Clear wake, state, and interrupts */
	mmio_w16(HDA_REG_STATESTS, mmio_r16(HDA_REG_STATESTS));
	mmio_w32(HDA_REG_INTSTS, mmio_r32(HDA_REG_INTSTS));
	mmio_w32(HDA_REG_INTCTL, 0);

	if (!hda_corb_rirb_init()) {
		return false;
	}

	/* Find first present codec (bits 0..2) */
	uint16_t ss = mmio_r16(HDA_REG_STATESTS);
	if (ss == 0) {
		/* some controllers need a nudge; read again */
		tiny_delay();
		ss = mmio_r16(HDA_REG_STATESTS);
	}
	if (ss == 0) {
		dprintf("hda: no codecs present (STATESTS=0)\n");
		return false;
	}
	for (uint8_t i = 0; i < 3; i++) {
		if (ss & (1u << i)) {
			hda.cad = i;
			break;
		}
	}

	/* Walk to AFG and select output path */
	nid_range_t wr;
	if (!hda_find_afg(hda.cad, &hda.afg_nid, &wr)) {
		dprintf("hda: no AFG found\n");
		return false;
	}
	if (!hda_pick_output_path(hda.cad, wr, &hda.dac_nid, &hda.pin_nid)) {
		dprintf("hda: no suitable output path\n");
		return false;
	}

	/* Power up AFG, DAC, PIN to D0 */
	hda_cmd(hda.cad, hda.afg_nid, V_SET_POWER_STATE, 0, NULL);
	hda_cmd(hda.cad, hda.dac_nid, V_SET_POWER_STATE, 0, NULL);
	hda_cmd(hda.cad, hda.pin_nid, V_SET_POWER_STATE, 0, NULL);

	/* Enable PIN: OUT_EN, and EAPD (bit1) */
	hda_cmd(hda.cad, hda.pin_nid, V_SET_PIN_WCTRL, 0x40, NULL);
	hda_cmd(hda.cad, hda.pin_nid, V_SET_EAPD_BTL, 0x02, NULL);

	/* Choose output SD index 0 (first output stream) and tag 1 */
	uint16_t gcap = mmio_r16(HDA_REG_GCAP);
	uint8_t n_in  = (gcap >> 8) & 0x0Fu;
	/* SD blocks start at 0x80; output streams follow inputs */
	hda.out_sd_index  = 0;
	hda.out_stream_tag = 1;

	/* Bind converter to our stream tag, channel 0 (left). We'll set channel count later. */
	hda_cmd(hda.cad, hda.dac_nid, V_SET_CONV_STREAMCH, (uint16_t)((hda.out_stream_tag << 4) | 0), NULL);
	/* Two channels */
	hda_cmd(hda.cad, hda.dac_nid, V_SET_CHANNEL_COUNT, 1, NULL);

	return true;
}

/* Compute base address of our output SD register block */
static inline uint32_t hda_out_sd_base(void) {
	uint16_t gcap = mmio_r16(HDA_REG_GCAP);
	uint8_t n_in  = (gcap >> 8) & 0x0Fu;
	return HDA_SD_BASE + (uint32_t)(n_in + hda.out_sd_index) * HDA_SD_STRIDE;
}

/* Reset a stream descriptor cleanly */
static void hda_sd_reset(uint32_t sdbase) {
	/* stop */
	uint8_t ctl0 = mmio_r8(sdbase + SD_CTL0);
	ctl0 &= ~SD_CTL_RUN;
	mmio_w8(sdbase + SD_CTL0, ctl0);
	/* reset */
	ctl0 |= SD_CTL_SRST;
	mmio_w8(sdbase + SD_CTL0, ctl0);
	for (int i = 0; i < 1000 && !(mmio_r8(sdbase + SD_CTL0) & SD_CTL_SRST); i++) { tiny_delay(); }
	ctl0 &= ~SD_CTL_SRST;
	mmio_w8(sdbase + SD_CTL0, ctl0);
	for (int i = 0; i < 1000 && (mmio_r8(sdbase + SD_CTL0) & SD_CTL_SRST); i++) { tiny_delay(); }
	/* clear status */
	mmio_w8(sdbase + SD_STS, mmio_r8(sdbase + SD_STS));
}

/* ---------- Stream / BDL preparation ---------- */
static inline uint8_t ring_free32(uint8_t civ, int tail) {
	if (tail < 0) {
		return 32;
	}
	int free = (int)civ - tail - 1;
	while (free < 0) {
		free += 32;
	}
	return (uint8_t)free;
}

/* Prepare BDL + buffer, set SDn registers and codec format. */
static bool hda_stream_prepare(uint32_t frag_bytes, uint32_t rate_hz) {
	if (frag_bytes == 0) {
		frag_bytes = 4096;
	}
	/* HDA requires BDL entries multiple of 128 bytes, and BDL base 128-aligned */
	frag_bytes = (frag_bytes + 127u) & ~127u;
	if (frag_bytes & 3u) {
		frag_bytes += (4u - (frag_bytes & 3u)); /* stereo 16-bit alignment */
	}

	const uint32_t bdl_n = 32;
	const uint32_t total_bytes = bdl_n * frag_bytes;

	uint8_t *buf_raw = kmalloc_low(total_bytes + 128);
	hda_bdl_entry_t *bdl_raw = kmalloc_low(sizeof(hda_bdl_entry_t) * bdl_n + 128);
	if (!buf_raw || !bdl_raw) {
		dprintf("hda: OOM allocating BDL/audio\n");
		return false;
	}
	uintptr_t buf_al = ((uintptr_t)buf_raw + 127u) & ~(uintptr_t)127u;
	uintptr_t bdl_al = ((uintptr_t)bdl_raw + 127u) & ~(uintptr_t)127u;

	hda.buf       = (uint8_t *)buf_al;
	hda.buf_phys  = (uint32_t)buf_al;
	hda.buf       = (uint8_t *)buf_al;
	hda.bdl       = (hda_bdl_entry_t *)bdl_al;
	hda.bdl_phys  = (uint32_t)bdl_al;
	hda.bdl_n     = bdl_n;
	hda.frag_bytes  = frag_bytes;
	hda.frag_frames = frag_bytes / 4u; /* 4 bytes per frame (stereo S16) */
	hda.tail        = -1;
	hda.started     = false;
	hda.rate_hz     = 44100;

	memset(hda.buf, 0, total_bytes);

	for (uint32_t i = 0; i < bdl_n; i++) {
		hda.bdl[i].addr  = (uint64_t)(hda.buf_phys + i * hda.frag_bytes);
		hda.bdl[i].len   = hda.frag_bytes;
		hda.bdl[i].flags = 0; /* IOC=0 (polled) */
	}

	/* Program stream descriptor */
	uint32_t sdbase = hda_out_sd_base();

	hda_sd_reset(sdbase);

	/* SDnBDP */
	mmio_w32(sdbase + SD_BDPL, hda.bdl_phys);
	mmio_w32(sdbase + SD_BDPU, 0);

	/* CBL = total bytes in cyclic buffer; LVI = last index (0..255), must be >= 1 */
	mmio_w32(sdbase + SD_CBL, total_bytes);
	mmio_w16(sdbase + SD_LVI, (uint16_t)(hda.bdl_n - 1u));

	/* Program 48 kHz, 16-bit, 2-ch in both codec and SDnFMT */
	uint16_t fmt = (uint16_t)(SD_FMT_BASE_44K1 | SD_FMT_MULT_X1 | SD_FMT_DIV_1 | SD_FMT_BITS_16 | SD_FMT_CHANS(2));
	/* Converter format (verb uses same field encoding) */
	hda_cmd(hda.cad, hda.dac_nid, 0x002, fmt, NULL);
	/* Stream format register */
	mmio_w16(sdbase + SD_FMT, fmt);

	/* SD_CTL: set stream tag (byte2[7:4]); leave RUN=0 for now */
	uint8_t ctl2 = (uint8_t)((hda.out_stream_tag & 0x0Fu) << 4);
	mmio_w8(sdbase + SD_CTL2, ctl2);

	return true;
}

/* Start the output engine */
static void hda_run_stream(void) {
	uint32_t sdbase = hda_out_sd_base();
	uint8_t ctl0 = mmio_r8(sdbase + SD_CTL0);
	ctl0 |= SD_CTL_RUN;
	mmio_w8(sdbase + SD_CTL0, ctl0);
}

/* CIV equivalent: read current buffer index from LPIB / bytes progressed → fragment index.
   HDA doesn't expose CIV directly; we derive it. */
static inline uint8_t hda_current_index(void) {
	uint32_t sdbase = hda_out_sd_base();
	uint32_t lpib = mmio_r32(sdbase + SD_LPIB); /* bytes since cycle start */
	return (uint8_t)((lpib / hda.frag_bytes) & 31u);
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
		uint8_t next = (uint8_t)((hda.tail + 1) & 31);
		uint8_t *dst = hda.buf + (uint32_t)next * hda.frag_bytes;

		size_t chunk_frames = hda.frag_frames;
		if (chunk_frames > frame_count) {
			chunk_frames = frame_count;
		}

		size_t bytes_copy = chunk_frames * 4u;
		memcpy(dst, frames + frames_done * 2u, bytes_copy);
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

/* ---------- SW FIFO (same pattern as AC'97 module) ---------- */
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
	int16_t *new_buf = kmalloc(new_cap * 2u * sizeof(int16_t));
	if (!new_buf) {
		return false;
	}
	if (live) {
		memcpy(new_buf, s_q_pcm + (s_q_head_fr * 2u), live * 2u * sizeof(int16_t));
	}
	if (s_q_pcm) {
		kfree(s_q_pcm);
	}
	s_q_pcm   = new_buf;
	s_q_cap_fr = new_cap;
	s_q_len_fr = live;
	s_q_head_fr = 0;
	return true;
}

static size_t push_all_s16le(const int16_t *frames, size_t total_frames) {
	if (!frames || total_frames == 0) {
		return 0;
	}
	if (!q_ensure_cap(total_frames)) {
		dprintf("hda: queue OOM (wanted %lu frames)\n", total_frames);
		return 0;
	}
	size_t live = s_q_len_fr - s_q_head_fr;
	memcpy(s_q_pcm + (live * 2u), frames, total_frames * 2u * sizeof(int16_t));
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
		size_t pushed = hda_play_s16le(s_q_pcm + (s_q_head_fr * 2u), chunk);
		if (pushed == 0) {
			break; /* ring full now */
		}
		s_q_head_fr += pushed;
		pending     -= pushed;
	}
	if (s_q_head_fr == s_q_len_fr) {
		s_q_head_fr = 0;
		s_q_len_fr  = 0;
	}

	/* If the stream halted due to underrun, restart cheaply */
	uint32_t sdbase = hda_out_sd_base();
	uint8_t ctl0 = mmio_r8(sdbase + SD_CTL0);
	if (!(ctl0 & SD_CTL_RUN) && !s_paused) {
		hda_run_stream();
		hda.started = true;
	}
}

/* Pause / resume / stop */
static void hda_pause(void) {
	uint32_t sdbase = hda_out_sd_base();
	uint8_t ctl0 = mmio_r8(sdbase + SD_CTL0);
	ctl0 &= ~SD_CTL_RUN;
	mmio_w8(sdbase + SD_CTL0, ctl0);
	s_paused = true;
}

static void hda_resume(void) {
	uint32_t sdbase = hda_out_sd_base();
	/* no special status to clear on SD; just RUN */
	hda_run_stream();
	s_paused = false;
	hda.started = true;
	hda_idle();
}

static void hda_stop_clear(void) {
	uint32_t sdbase = hda_out_sd_base();
	uint8_t ctl0 = mmio_r8(sdbase + SD_CTL0);
	ctl0 &= ~SD_CTL_RUN;
	mmio_w8(sdbase + SD_CTL0, ctl0);

	hda_sd_reset(sdbase);

	s_q_head_fr = 0;
	s_q_len_fr  = 0;
	s_paused    = false;
	hda.started = false;
}

/* Buffered milliseconds: SW + HW (LPIB position inside total cycle) */
static uint32_t hda_buffered_ms(void) {
	if (!hda.frag_frames || !hda.rate_hz) {
		return 0;
	}
	size_t sw_frames = s_q_len_fr - s_q_head_fr;

	uint32_t sdbase = hda_out_sd_base();
	uint32_t lpib   = mmio_r32(sdbase + SD_LPIB);
	uint32_t cbl    = mmio_r32(sdbase + SD_CBL);

	size_t hw_bytes_left = (size_t)cbl - (size_t)lpib;
	size_t hw_frames     = hw_bytes_left / 4u;

	size_t total_frames  = sw_frames + hw_frames;
	return (uint32_t)((total_frames * 1000ull) / hda.rate_hz);
}

static uint32_t hda_get_hz(void) {
	return 44100;
}

/* ---------- Module init / registration ---------- */
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
	hda_cmd(hda.cad, hda.dac_nid, V_SET_CONV_STREAMCH, (uint16_t)((hda.out_stream_tag << 4) | 0), NULL);

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
	kprintf("hda: started (cad=%u, afg=0x%02x, dac=0x%02x, pin=0x%02x)\n",
		hda.cad, hda.afg_nid, hda.dac_nid, hda.pin_nid);
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	/* not unloadable yet */
	return false;
}
