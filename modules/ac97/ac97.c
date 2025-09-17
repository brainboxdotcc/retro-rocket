#include <kernel.h>
#include "ac97.h"

static ac97_dev_t g;
/* software FIFO: interleaved S16LE stereo, counted in FRAMES (not samples) */
static int16_t *s_q_pcm      = NULL;   /* backing buffer (L,R interleaved) */
static size_t   s_q_cap_fr   = 0;      /* capacity in frames */
static size_t   s_q_len_fr   = 0;      /* total frames in buffer (valid) */
static size_t   s_q_head_fr  = 0;      /* consume pointer in frames */

bool ac97_stream_prepare(uint32_t frag_bytes /*=0→4096*/, uint32_t sample_rate /*e.g. 48000*/);
void ac97_idle(void);

static inline void delay_us(unsigned us) {
	/* Approximate microsecond delay using io_wait().
	   Each io_wait is typically ~1–4 µs on PC hardware. */
	for (unsigned i = 0; i < us; i++) {
		io_wait();
	}
}

static bool ac97_reset_and_unmute(void) {
	/* Exit cold reset (run): set bit1 in GLOB_CNT. */
	outl(g.nabm + AC97_NABM_GLOB_CNT, 0x00000002);

	/* Soft reset codec registers (write-anything) */
	outw(g.nam + AC97_NAM_RESET, 0x0000);

	/* Unmute + max out */
	outw(g.nam + AC97_NAM_MASTER_VOL, 0x0000);
	outw(g.nam + AC97_NAM_PCM_OUT_VOL, 0x0000);

	/* If variable-rate capable, enable and set 48k (harmless if absent) */
	uint16_t caps = inw(g.nam + AC97_NAM_EXT_CAPS);
	if (caps & 0x0001) {
		uint16_t ec = inw(g.nam + AC97_NAM_EXT_CTRL);
		outw(g.nam + AC97_NAM_EXT_CTRL, ec | 0x0001);
		outw(g.nam + AC97_NAM_PCM_FRONT_RATE, 48000);
	}
	return true;
}

static void ac97_start_po(void) {
	/* Reset PCM-OUT box */
	outb(g.nabm + AC97_NABM_PO_CR, AC97_CR_RST);
	for (int i = 0; i < 1000 && (inb(g.nabm + AC97_NABM_PO_CR) & AC97_CR_RST); i++) {
		delay_us(10);
	}

	/* Clear stale status */
	outw(g.nabm + AC97_NABM_PO_SR, AC97_SR_W1C_MASK);

	/* Program BDL and mark only entry 0 valid */
	outl(g.nabm + AC97_NABM_PO_BDBAR, g.bdl_phys);
	outb(g.nabm + AC97_NABM_PO_LVI, 0);

	/* Run */
	outb(g.nabm + AC97_NABM_PO_CR, AC97_CR_RPBM);
}

/* ===================== Probe & start ===================== */
static bool ac97_start(pci_dev_t dev) {
	uint32_t bar0 = pci_read(dev, PCI_BAR0);
	uint32_t bar1 = pci_read(dev, PCI_BAR1);

	if (pci_bar_type(bar0) != PCI_BAR_TYPE_IOPORT || pci_bar_type(bar1) != PCI_BAR_TYPE_IOPORT) {
		dprintf("ac97: expected I/O BARs for NAM/NABM\n");
		return false;
	}

	g.nam = (uint16_t) pci_io_base(bar0);
	g.nabm = (uint16_t) pci_io_base(bar1);

	pci_bus_master(dev);

	ac97_reset_and_unmute();

	ac97_start_po();

	uint8_t civ = inb(g.nabm + AC97_NABM_PO_CIV);
	uint8_t lvi = inb(g.nabm + AC97_NABM_PO_LVI);
	uint16_t picb = inw(g.nabm + AC97_NABM_PO_PICB);
	dprintf("ac97: NAM=0x%04x NABM=0x%04x | CIV=%u LVI=%u PICB=%u(samples)\n", g.nam, g.nabm, civ, lvi, picb);

	return true;
}

static bool init_ac97(void) {
	pci_dev_t dev = pci_get_device(0, 0, 0x0401);
	if (!dev.bits) {
		dprintf("ac97: no devices (class 0x0401)\n");
		return false;
	}
	if (ac97_start(dev)) {
		kprintf("ac97: started\n");
		ac97_stream_prepare(4096 /*frag bytes*/, 48000 /*Hz*/);
	} else {
		dprintf("ac97: start failed\n");
		return false;
	}
	proc_register_idle(ac97_idle, IDLE_FOREGROUND, 1);
	return true;
}

/* ---- helpers ---- */
static inline uint8_t ac97_po_civ(void) {
	return inb(g.nabm + AC97_NABM_PO_CIV);
}

static inline void ac97_po_set_lvi(uint8_t v) {
	outb(g.nabm + AC97_NABM_PO_LVI, v);
}

static inline void ac97_po_run(void) {
	outb(g.nabm + AC97_NABM_PO_CR, AC97_CR_RPBM);
}

static inline void ac97_po_reset_box(void) {
	outb(g.nabm + AC97_NABM_PO_CR, AC97_CR_RST);
	for (int i = 0; i < 1000 && (inb(g.nabm + AC97_NABM_PO_CR) & AC97_CR_RST); i++) {
		delay_us(10);
	}
}

/* free slots in a 32-entry ring:
   CIV = hardware “current” (the entry being played),
   tail = our last valid index (−1 means ring empty). */
static inline uint8_t ring_free32(uint8_t civ, int tail) {
	if (tail < 0) {
		return 32;                     /* nothing queued yet */
	}
	int free = (int) civ - tail - 1;              /* keep one slot gap */
	while (free < 0) free += 32;
	return (uint8_t) free;
}

/* ======================== one-time streaming setup ======================== */
/* Build a 32-entry BDL and a contiguous audio buffer, then point the PO box at it.
   frag_bytes defaults to 4096 if 0 is passed; must be multiple of 128 and 4. */
bool ac97_stream_prepare(uint32_t frag_bytes, uint32_t sample_rate) {
	if (!g.nam || !g.nabm) {
		dprintf("ac97: prepare: device not started\n");
		return false;
	}

	/* Normalise fragment size */
	if (frag_bytes == 0) frag_bytes = 4096;
	/* multiple of 128 (DMA) */
	frag_bytes = (frag_bytes + 127u) & ~127u;
	/* multiple of 4 (stereo 16-bit) */
	if (frag_bytes & 3u) frag_bytes += (4u - (frag_bytes & 3u));

	/* Program 48k (or requested) if var-rate capable */
	uint16_t caps = inw(g.nam + AC97_NAM_EXT_CAPS);
	if (caps & 0x0001) {
		uint16_t ec = inw(g.nam + AC97_NAM_EXT_CTRL);
		outw(g.nam + AC97_NAM_EXT_CTRL, ec | 0x0001);
		outw(g.nam + AC97_NAM_PCM_FRONT_RATE, (uint16_t) sample_rate);
	}

	/* Allocate BDL (32 entries) and a single contiguous audio area: 32 * frag_bytes */
	const uint32_t bdl_n = 32;
	const uint32_t total_bytes = bdl_n * frag_bytes;

	uint8_t *buf_raw = kmalloc_low(total_bytes + 128);
	struct ac97_bdl_entry *bdl_raw = kmalloc_low(sizeof(struct ac97_bdl_entry) * bdl_n + 128);
	if (!buf_raw || !bdl_raw) {
		dprintf("ac97: prepare: OOM\n");
		return false;
	}

	uintptr_t buf_al = ((uintptr_t) buf_raw + 127u) & ~(uintptr_t) 127u;
	uintptr_t bdl_al = ((uintptr_t) bdl_raw + 127u) & ~(uintptr_t) 127u;

	g.buf = (uint8_t *) buf_al;
	g.buf_phys = (uint32_t) buf_al;
	g.buf_bytes = total_bytes;

	g.bdl = (struct ac97_bdl_entry *) bdl_al;
	g.bdl_phys = (uint32_t) bdl_al;

	g.bdl_n = 32;
	g.frag_bytes = frag_bytes;
	g.frag_frames = frag_bytes / 4u;          /* 4 bytes per frame (L,R S16) */
	g.tail = -1;                       /* nothing queued yet */
	g.started = false;

	memset(g.buf, 0, total_bytes);

	/* Populate BDL entries: contiguous fragments */
	const uint16_t samples_per_desc = (uint16_t) (g.frag_frames * 2u); /* samples = frames*channels */
	for (uint32_t i = 0; i < g.bdl_n; i++) {
		g.bdl[i].addr = g.buf_phys + i * g.frag_bytes;
		g.bdl[i].count = samples_per_desc;
		g.bdl[i].flags = 0;                  /* no IOC, continuous */
	}

	/* Point PO engine at our BDL */
	ac97_po_reset_box();
	outw(g.nabm + AC97_NABM_PO_SR, AC97_SR_W1C_MASK); /* clear stale */
	outl(g.nabm + AC97_NABM_PO_BDBAR, g.bdl_phys);

	/* Don’t start yet; we’ll set RPBM on first queue */
	return true;
}

/* ======================== non-blocking enqueue ======================== */
/* Queues S16LE stereo frames into the ring. Returns number of frames accepted.
   Zero-fills any tail of a fragment if the caller provides fewer than frag_frames. */
size_t ac97_play_s16le(const int16_t *frames, size_t frame_count) {
	if (!g.bdl || !g.buf || g.bdl_n != 32 || g.frag_frames == 0) return 0;

	uint8_t civ = ac97_po_civ();
	uint8_t free = ring_free32(civ, g.tail);

	size_t frames_done = 0;
	while (free && frame_count) {
		uint8_t next = (uint8_t) ((g.tail + 1) & 31);   /* modulo 32 */
		uint8_t *dst = g.buf + (uint32_t) next * g.frag_bytes;

		size_t chunk_frames = g.frag_frames;
		if (chunk_frames > frame_count) chunk_frames = frame_count;

		/* copy caller frames, then zero-pad the rest of the fragment */
		size_t bytes_copy = chunk_frames * 4u;
		memcpy(dst, frames + frames_done * 2u, bytes_copy);
		if (bytes_copy < g.frag_bytes) {
			memset(dst + bytes_copy, 0, g.frag_bytes - bytes_copy);
		}

		/* publish: move LVI to this index */
		g.tail = next;
		ac97_po_set_lvi((uint8_t) g.tail);

		/* start engine if needed */
		if (!g.started) {
			ac97_po_run();
			g.started = true;
		}

		frames_done += chunk_frames;
		frame_count -= chunk_frames;
		free--;
	}

	return frames_done;
}

/* ensure capacity for at least extra_fr more frames; returns false on OOM */
static bool q_ensure_cap(size_t extra_fr) {
	size_t live = s_q_len_fr - s_q_head_fr;
	size_t need = live + extra_fr;
	if (need <= s_q_cap_fr) {
		return true;
	}

	/* grow: double or to need, whichever larger */
	size_t new_cap = s_q_cap_fr ? s_q_cap_fr : 4096;
	while (new_cap < need) {
		new_cap <<= 1;
	}

	int16_t *new_buf = kmalloc(new_cap * 2u * sizeof(int16_t)); /* 2 samples/frame */
	if (!new_buf) {
		return false;
	}

	/* compact live frames to start */
	if (live) {
		memcpy(new_buf, s_q_pcm + (s_q_head_fr * 2u), live * 2u * sizeof(int16_t));
	}
	if (s_q_pcm) {
		kfree(s_q_pcm);
	}
	s_q_pcm     = new_buf;
	s_q_cap_fr  = new_cap;
	s_q_len_fr  = live;
	s_q_head_fr = 0;
	return true;
}

/* NON-BLOCKING: enqueue frames into SW FIFO; the idle hook will drain them.
   Returns the number of frames accepted into the queue (== total_frames on success). */
size_t push_all_s16le(const int16_t *frames, size_t total_frames) {
	if (!frames || total_frames == 0) return 0;
	if (!q_ensure_cap(total_frames)) {
		dprintf("ac97: queue OOM (wanted %lu frames)\n", total_frames);
		return 0;
	}

	/* append to tail */
	size_t live = s_q_len_fr - s_q_head_fr;
	memcpy(s_q_pcm + (live * 2u), frames, total_frames * 2u * sizeof(int16_t));
	s_q_len_fr = live + total_frames;

	return total_frames;
}

/* idle hook: drain SW FIFO into AC’97 ring without blocking */
void ac97_idle(void) {
	if (!g.bdl || g.frag_frames == 0) return;

	/* number of frames pending in SW FIFO */
	size_t pending = s_q_len_fr - s_q_head_fr;
	if (pending == 0) return;

	/* push as long as hardware accepts data this instant */
	while (pending) {
		size_t chunk = (pending > g.frag_frames) ? g.frag_frames : pending;
		size_t pushed = ac97_play_s16le(s_q_pcm + (s_q_head_fr * 2u), chunk);
		if (pushed == 0) break;                 /* ring full right now; come back next idle tick */

		s_q_head_fr += pushed;
		pending     -= pushed;
	}

	/* compact when fully drained to keep indices small */
	if (s_q_head_fr == s_q_len_fr) {
		s_q_head_fr = 0;
		s_q_len_fr  = 0;
		/* (buffer kept for reuse; free here if you want to release memory when idle) */
		/* if (s_q_cap_fr > SOME_LIMIT) { kfree(s_q_pcm); s_q_pcm=NULL; s_q_cap_fr=0; } */
	}

	/* recover from a halted engine if needed (cheap check) */
	uint16_t sr = inw(g.nabm + AC97_NABM_PO_SR);
	if (sr & (AC97_SR_DCH | AC97_SR_CELV)) {
		outw(g.nabm + AC97_NABM_PO_SR, AC97_SR_W1C_MASK);
		outb(g.nabm + AC97_NABM_PO_CR, AC97_CR_RPBM);
	}
}


void ac97_test_melody(void) {
	if (!g.bdl || !g.buf || g.bdl_n != 32 || g.frag_frames == 0 || g.frag_bytes == 0) {
		dprintf("ac97: test_melody: stream not prepared\n");
		return;
	}

	/* One ephemeral staging fragment (stereo S16LE). No special alignment needed. */
	int16_t *frag = kmalloc(g.frag_bytes);
	if (!frag) {
		dprintf("ac97: test_melody: OOM\n");
		return;
	}

	/* Use actual programmed rate if var-rate is enabled; otherwise assume 48k. */
	uint32_t rate = 48000;
	uint16_t caps = inw(g.nam + AC97_NAM_EXT_CAPS);
	if (caps & 0x0001) {
		if (inw(g.nam + AC97_NAM_EXT_CTRL) & 0x0001) {
			rate = inw(g.nam + AC97_NAM_PCM_FRONT_RATE);
			if (rate == 0) rate = 48000;
		}
	}

	/* Little square-wave line: A4 B4 C5 E5 D5 C5 B4 A4 A5 (each ~60 ms) */
	static const uint16_t notes_hz[] = {440, 494, 523, 659, 587, 523, 494, 440, 880};
	static const uint16_t dur_ms[] = {600, 600, 600, 600, 600, 600, 600, 600, 600};
	const size_t nnotes = sizeof(notes_hz) / sizeof(notes_hz[0]);

	const uint32_t F = g.frag_frames;   /* frames per fragment (stereo S16) */
	size_t total_frames_enqueued = 0;

	for (size_t n = 0; n < nnotes; n++) {
		const uint32_t freq = notes_hz[n];
		uint32_t frames_left = (rate / 1000u) * dur_ms[n];
		if (freq == 0) continue;

		/* Integer square-wave state */
		uint32_t period = rate / freq;
		if (period == 0) period = 1;
		uint32_t halfperiod = period >> 1;
		uint32_t phase = 0;

		while (frames_left > 0) {
			const uint32_t out_frames = (frames_left > F) ? F : frames_left;

			/* Generate exactly out_frames into the staging fragment */
			for (uint32_t i = 0; i < out_frames; i++) {
				const int16_t s = (phase < halfperiod) ? 6000 : (int16_t) -6000;
				frag[(i << 1)] = s;  /* L */
				frag[(i << 1) + 1] = s;  /* R */
				if (++phase >= period) phase = 0;
			}

			/* Non-blocking enqueue (zero-pads the tail of the fragment inside ac97_play_s16le) */
			const size_t pushed = push_all_s16le(frag, out_frames);
			if (pushed == 0 || pushed < out_frames) {
				dprintf("ac97: test_melody: ring full after %zu frames\n", total_frames_enqueued);
				kfree(frag);
				return;
			}

			total_frames_enqueued += pushed;
			frames_left -= out_frames;
		}
	}

	dprintf("ac97: test_melody: enqueued %zu frames (~%u ms)\n",
		total_frames_enqueued,
		(unsigned) ((total_frames_enqueued * 1000u) / rate));

	kfree(frag); /* safe: hardware reads from the DMA ring, not this staging buffer */
}

/* ===================== Module entry points ===================== */
bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("ac97: module loaded\n");
	if (init_ac97()) {
		ac97_test_melody();
	}
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	/* For a one-shot demo we don’t tear down; engine stops on its own */
	dprintf("ac97: module unloaded\n");
	return false;
}
