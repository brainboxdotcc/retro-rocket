/**
 * @file modules/ac97/ac97.c
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#include <kernel.h>
#include "ac97.h"

static ac97_dev_t ac97;
static int16_t *s_q_pcm = NULL;   /* backing buffer (L,R interleaved) */
static size_t s_q_cap_fr = 0;      /* capacity in frames */
static size_t s_q_len_fr = 0;      /* total frames in buffer (valid) */
static size_t s_q_head_fr = 0;      /* consume pointer in frames */
static bool s_paused = false;

static inline void delay_us(unsigned us) {
	for (unsigned i = 0; i < us; i++) {
		io_wait();
	}
}

static bool ac97_reset_and_unmute(void) {
	outl(ac97.nabm + AC97_NABM_GLOB_CNT, 0x00000002);
	outw(ac97.nam + AC97_NAM_RESET, 0x0000);
	outw(ac97.nam + AC97_NAM_MASTER_VOL, 0x0000);
	outw(ac97.nam + AC97_NAM_PCM_OUT_VOL, 0x0000);
	uint16_t caps = inw(ac97.nam + AC97_NAM_EXT_CAPS);
	if (caps & 0x0001) {
		uint16_t ec = inw(ac97.nam + AC97_NAM_EXT_CTRL);
		outw(ac97.nam + AC97_NAM_EXT_CTRL, ec | 0x0001);
		outw(ac97.nam + AC97_NAM_PCM_FRONT_RATE, 44100);
		dprintf("ac97: PCM front DAC rate now %u Hz\n", inw(ac97.nam + AC97_NAM_PCM_FRONT_RATE));
	}
	return true;
}

static void ac97_start_po(void) {
	outb(ac97.nabm + AC97_NABM_PO_CR, AC97_CR_RST);
	for (int i = 0; i < 1000 && (inb(ac97.nabm + AC97_NABM_PO_CR) & AC97_CR_RST); i++) {
		delay_us(10);
	}
	outw(ac97.nabm + AC97_NABM_PO_SR, AC97_SR_W1C_MASK);
	outl(ac97.nabm + AC97_NABM_PO_BDBAR, ac97.bdl_phys);
	outb(ac97.nabm + AC97_NABM_PO_LVI, 0);
	outb(ac97.nabm + AC97_NABM_PO_CR, AC97_CR_RPBM);
}

static bool ac97_start(pci_dev_t dev) {
	uint32_t bar0 = pci_read(dev, PCI_BAR0);
	uint32_t bar1 = pci_read(dev, PCI_BAR1);

	if (pci_bar_type(bar0) != PCI_BAR_TYPE_IOPORT || pci_bar_type(bar1) != PCI_BAR_TYPE_IOPORT) {
		dprintf("ac97: expected I/O BARs for NAM/NABM\n");
		return false;
	}

	ac97.nam = pci_io_base(bar0);
	ac97.nabm = pci_io_base(bar1);

	if (!pci_bus_master(dev)) {
		dprintf("ac97: failed to set bus master\n");
		return false;
	}
	ac97_reset_and_unmute();
	ac97_start_po();

	uint8_t civ = inb(ac97.nabm + AC97_NABM_PO_CIV);
	uint8_t lvi = inb(ac97.nabm + AC97_NABM_PO_LVI);
	uint16_t picb = inw(ac97.nabm + AC97_NABM_PO_PICB);
	dprintf("ac97: NAM=0x%04x NABM=0x%04x | CIV=%u LVI=%u PICB=%u(samples)\n", ac97.nam, ac97.nabm, civ, lvi, picb);

	return true;
}

static const char** ac97_list_output_names(void) {
	return (const char*[2]){
		"Line Out",
		NULL
	};
}

static bool ac97_select_output_by_name(const char *name) {
	if (!name) {
		return false;
	}
	return (strcasecmp(name, "Line Out") == 0);
}

static const char* ac97_get_current_output(void) {
	return "Line Out";
}

static audio_device_t *init_ac97(void) {
	pci_dev_t dev = pci_get_device(0, 0, 0x0401);
	if (!dev.bits) {
		dprintf("ac97: no devices (class 0x0401)\n");
		return NULL;
	}
	if (ac97_start(dev)) {
		kprintf("ac97: started\n");
		ac97_stream_prepare(4096, 44100); /* 44.1khz stereo LE 16 bit */
	} else {
		dprintf("ac97: start failed\n");
		return NULL;
	}
	proc_register_idle(ac97_idle, IDLE_FOREGROUND, 1);

	audio_device_t *device = kmalloc(sizeof(audio_device_t));
	make_unique_device_name("audio", device->name, MAX_AUDIO_DEVICE_NAME);
	device->opaque = &ac97;
	device->next = NULL;
	device->play = push_all_s16le;
	device->frequency = ac97_get_hz;
	device->pause = ac97_pause;
	device->resume = ac97_resume;
	device->stop = ac97_stop_clear;
	device->queue_length = ac97_buffered_ms;
	device->get_outputs = ac97_list_output_names;
	device->select_output = ac97_select_output_by_name;
	device->get_current_output = ac97_get_current_output;

	return register_audio_device(device) ? device : NULL;
}

static inline uint8_t ac97_po_civ(void) {
	return inb(ac97.nabm + AC97_NABM_PO_CIV);
}

static inline void ac97_po_set_lvi(uint8_t v) {
	outb(ac97.nabm + AC97_NABM_PO_LVI, v);
}

static inline void ac97_po_run(void) {
	outb(ac97.nabm + AC97_NABM_PO_CR, AC97_CR_RPBM);
}

static inline void ac97_po_reset_box(void) {
	outb(ac97.nabm + AC97_NABM_PO_CR, AC97_CR_RST);
	for (int i = 0; i < 1000 && (inb(ac97.nabm + AC97_NABM_PO_CR) & AC97_CR_RST); i++) {
		delay_us(10);
	}
}

/* free slots in a 32-entry ring:
   CIV = hardware “current” (the entry being played),
   tail = our last valid index (−1 means ring empty). */
static inline uint8_t ring_free32(uint8_t civ, int tail) {
	if (tail < 0) {
		return 32; /* nothing queued yet */
	}
	int free = (int) civ - tail - 1; /* keep one slot gap */
	while (free < 0) free += 32;
	return (uint8_t) free;
}

/* Build a 32-entry BDL and a contiguous audio buffer, then point the PO box at it.
   frag_bytes defaults to 4096 if 0 is passed; must be multiple of 128 and 4. */
bool ac97_stream_prepare(uint32_t frag_bytes, uint32_t sample_rate) {
	if (!ac97.nam || !ac97.nabm) {
		dprintf("ac97: prepare: device not started\n");
		return false;
	}

	/* Normalise fragment size */
	if (frag_bytes == 0) frag_bytes = 4096;
	/* multiple of 128 (DMA) */
	frag_bytes = (frag_bytes + 127u) & ~127u;
	/* multiple of 4 (stereo 16-bit) */
	if (frag_bytes & 3u) frag_bytes += (4u - (frag_bytes & 3u));

	/* Program 44.1khz (or requested) if var-rate capable */
	uint16_t caps = inw(ac97.nam + AC97_NAM_EXT_CAPS);
	if (caps & 0x0001) {
		uint16_t ec = inw(ac97.nam + AC97_NAM_EXT_CTRL);
		outw(ac97.nam + AC97_NAM_EXT_CTRL, ec | 0x0001);
		outw(ac97.nam + AC97_NAM_PCM_FRONT_RATE, (uint16_t) sample_rate);
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

	ac97.buf = (uint8_t *) buf_al;
	ac97.buf_phys = (uint32_t) buf_al;
	ac97.buf_bytes = total_bytes;

	ac97.bdl = (struct ac97_bdl_entry *) bdl_al;
	ac97.bdl_phys = (uint32_t) bdl_al;

	ac97.bdl_n = 32;
	ac97.frag_bytes = frag_bytes;
	ac97.frag_frames = frag_bytes / 4u;          /* 4 bytes per frame (L,R S16) */
	ac97.tail = -1;                       /* nothing queued yet */
	ac97.started = false;
	ac97.rate_hz = 44100;

	memset(ac97.buf, 0, total_bytes);

	/* Populate BDL entries: contiguous fragments */
	const uint16_t samples_per_desc = (uint16_t) (ac97.frag_frames * 2u); /* samples = frames*channels */
	for (uint32_t i = 0; i < ac97.bdl_n; i++) {
		ac97.bdl[i].addr = ac97.buf_phys + i * ac97.frag_bytes;
		ac97.bdl[i].count = samples_per_desc;
		ac97.bdl[i].flags = 0;                  /* no IOC, continuous */
	}

	/* Point PO engine at our BDL */
	ac97_po_reset_box();
	outw(ac97.nabm + AC97_NABM_PO_SR, AC97_SR_W1C_MASK); /* clear stale */
	outl(ac97.nabm + AC97_NABM_PO_BDBAR, ac97.bdl_phys);

	/* Don’t start yet; we’ll set RPBM on first queue */
	return true;
}

/* Queues S16LE stereo frames into the AC97. Returns number of frames accepted.
   Zero-fills any tail of a fragment if the caller provides fewer than frag_frames. */
size_t ac97_play_s16le(const int16_t *frames, size_t frame_count) {
	if (!ac97.bdl || !ac97.buf || ac97.bdl_n != 32 || ac97.frag_frames == 0) return 0;

	uint8_t civ = ac97_po_civ();
	uint8_t free = ring_free32(civ, ac97.tail);

	size_t frames_done = 0;
	while (free && frame_count) {
		uint8_t next = (uint8_t) ((ac97.tail + 1) & 31);   /* modulo 32 */
		uint8_t *dst = ac97.buf + (uint32_t) next * ac97.frag_bytes;

		size_t chunk_frames = ac97.frag_frames;
		if (chunk_frames > frame_count) chunk_frames = frame_count;

		/* copy caller frames, then zero-pad the rest of the fragment */
		size_t bytes_copy = chunk_frames * 4u;
		memcpy(dst, frames + frames_done * 2u, bytes_copy);
		if (bytes_copy < ac97.frag_bytes) {
			memset(dst + bytes_copy, 0, ac97.frag_bytes - bytes_copy);
		}

		/* publish: move LVI to this index */
		ac97.tail = next;
		ac97_po_set_lvi((uint8_t) ac97.tail);

		/* start engine if needed */
		if (!ac97.started) {
			ac97_po_run();
			ac97.started = true;
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
	s_q_pcm = new_buf;
	s_q_cap_fr = new_cap;
	s_q_len_fr = live;
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
	if (!ac97.bdl || ac97.frag_frames == 0 || s_paused) {
		return;
	}

	/* number of frames pending in SW FIFO */
	size_t pending = s_q_len_fr - s_q_head_fr;
	if (pending == 0) return;

	/* push as long as hardware accepts data this instant */
	while (pending) {
		size_t chunk = (pending > ac97.frag_frames) ? ac97.frag_frames : pending;
		size_t pushed = ac97_play_s16le(s_q_pcm + (s_q_head_fr * 2u), chunk);
		if (pushed == 0) break;                 /* ring full right now; come back next idle tick */

		s_q_head_fr += pushed;
		pending -= pushed;
	}

	/* compact when fully drained to keep indices small */
	if (s_q_head_fr == s_q_len_fr) {
		s_q_head_fr = 0;
		s_q_len_fr = 0;
	}

	/* recover from a halted engine if needed (cheap check) */
	uint16_t sr = inw(ac97.nabm + AC97_NABM_PO_SR);
	if (sr & (AC97_SR_DCH | AC97_SR_CELV)) {
		outw(ac97.nabm + AC97_NABM_PO_SR, AC97_SR_W1C_MASK);
		outb(ac97.nabm + AC97_NABM_PO_CR, AC97_CR_RPBM);
	}
}

/* Immediate hard stop: halt DMA, reset PO engine, clear SW queue. */
void ac97_stop_clear(void) {
	/* stop engine */
	outb(ac97.nabm + AC97_NABM_PO_CR, 0);

	/* reset the bus-master “PCM Out” box */
	outb(ac97.nabm + AC97_NABM_PO_CR, AC97_CR_RST);
	while (inb(ac97.nabm + AC97_NABM_PO_CR) & AC97_CR_RST) { /* tiny spin */ }

	/* clear stale status and re-point BDL (not strictly needed after RST, but tidy) */
	outw(ac97.nabm + AC97_NABM_PO_SR, AC97_SR_W1C_MASK);
	outl(ac97.nabm + AC97_NABM_PO_BDBAR, ac97.bdl_phys);

	/* software queue: drop everything */
	s_q_head_fr = 0;
	s_q_len_fr = 0;

	/* paused state and started flag */
	s_paused = false;
	ac97.started = false;
}

/* Soft pause: stop DMA, keep ring + SW queue intact. */
void ac97_pause(void) {
	/* stop engine, keep descriptors/data as-is */
	outb(ac97.nabm + AC97_NABM_PO_CR, 0);
	s_paused = true;
}

/* Resume after pause: run DMA again and try to push immediately. */
void ac97_resume(void) {
	/* clear any latched status and ensure RUN */
	outw(ac97.nabm + AC97_NABM_PO_SR, AC97_SR_W1C_MASK);
	outb(ac97.nabm + AC97_NABM_PO_CR, AC97_CR_RPBM);
	s_paused = false;
	ac97.started = true;
	/* opportunistic push; returns quickly if ring is full */
	ac97_idle();
}

/* Return total buffered audio (SW queue + HW DMA) in milliseconds. */
uint32_t ac97_buffered_ms(void) {
	if (!ac97.frag_frames || !ac97.rate_hz) {
		return 0;
	}

	/* software FIFO frames */
	size_t sw_frames = s_q_len_fr - s_q_head_fr;

	/* hardware: how many frames left in DMA? */
	uint16_t civ = inb(ac97.nabm + AC97_NABM_PO_CIV);   /* current index value */
	uint16_t lvi = inb(ac97.nabm + AC97_NABM_PO_LVI);   /* last valid index */
	uint16_t picb = inw(ac97.nabm + AC97_NABM_PO_PICB);  /* frames remaining in current buffer */

	/* work out how many fragments are still pending */
	int pending_frags = (lvi - civ) & (ac97.bdl_n - 1); /* ring buffer modulo */
	size_t hw_frames = (pending_frags * ac97.frag_frames) + picb;

	size_t total_frames = sw_frames + hw_frames;

	/* convert frames → milliseconds */
	uint32_t ms = (uint32_t) ((total_frames * 1000ULL) / ac97.rate_hz);
	return ms;
}

uint32_t ac97_get_hz() {
	uint32_t rate = 44100;
	uint16_t caps = inw(ac97.nam + AC97_NAM_EXT_CAPS);
	if (caps & 0x0001) {
		if (inw(ac97.nam + AC97_NAM_EXT_CTRL) & 0x0001) {
			rate = inw(ac97.nam + AC97_NAM_PCM_FRONT_RATE);
			if (rate == 0) {
				rate = 44100;
			}
		}
	}
	return rate;
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("ac97: module loaded\n");
	audio_device_t *dev;
	if (!(dev = init_ac97())) {
		return false;
	}
	if (!mixer_init(dev, 50, 25, 64)) {
		dprintf("ac97: mixer init failed\n");
		return false;
	}
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	return false;
}
