#include <kernel.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ===================== AC'97 I/O register layout ===================== */
/* BAR0 = NAM (Native Audio Mixer) — I/O ports */
#define AC97_NAM_RESET          0x00 /* write-anything: soft reset; read: caps */
#define AC97_NAM_MASTER_VOL     0x02 /* word: 0x0000=max, bit15=mute */
#define AC97_NAM_PCM_OUT_VOL    0x18 /* word: 0x0000=max, bit15=mute */
#define AC97_NAM_EXT_CAPS       0x28 /* word: bit0 = variable-rate capable */
#define AC97_NAM_EXT_CTRL       0x2A /* word: bit0 = enable variable-rate */
#define AC97_NAM_PCM_FRONT_RATE 0x2C /* word: sample rate when var-rate enabled */

/* BAR1 = NABM (Native Audio Bus Master) — I/O ports */
#define AC97_NABM_PO_BDBAR      0x10 /* dword: phys addr of BDL for PCM OUT */
#define AC97_NABM_PO_CIV        0x14 /*  u8  : current index (RO) */
#define AC97_NABM_PO_LVI        0x15 /*  u8  : last valid index (0..31) */
#define AC97_NABM_PO_SR         0x16 /* u16  : status (W1C bits) */
#define AC97_NABM_PO_PICB       0x18 /* u16  : samples left in current entry (RO) */
#define AC97_NABM_PO_CR         0x1B /*  u8  : control: bit1=RST, bit0=RPBM(run) */

#define AC97_NABM_GLOB_CNT      0x2C /* u32  : bit1=Cold Reset run (1=out of reset) */

/* PO_SR W1C bits (clear by writing 1s) */
#define AC97_SR_LVBCI           0x0004
#define AC97_SR_DCH             0x0008
#define AC97_SR_CELV            0x0010
#define AC97_SR_W1C_MASK        (AC97_SR_LVBCI | AC97_SR_DCH | AC97_SR_CELV)

/* PO_CR bits */
#define AC97_CR_RPBM            0x01
#define AC97_CR_RST             0x02

/* ===================== Simple BDL entry (8 bytes) ===================== */
struct ac97_bdl_entry {
	uint32_t addr;   /* phys addr of audio data */
	uint16_t count;  /* number of samples in this buffer (all channels) */
	uint16_t flags;  /* bit15=IOC, bit14=BUP (buffer underrun policy / stop) */
} __attribute__((packed));

/* ===================== Driver state ===================== */
typedef struct {
	uint16_t nam;          /* I/O base (NAM) */
	uint16_t nabm;         /* I/O base (NABM) */
	struct ac97_bdl_entry *bdl;
	uint32_t               bdl_phys;
	uint8_t               *buf;
	uint32_t               buf_phys;
	uint32_t               buf_bytes;
} ac97_dev_t;

static ac97_dev_t g;

static inline void delay_us(unsigned us) {
	/* Approximate microsecond delay using io_wait().
	   Each io_wait is typically ~1–4 µs on PC hardware. */
	for (unsigned i = 0; i < us; i++) {
		io_wait();
	}
}

/* ===================== Helpers ===================== */
static void fill_square_440(uint8_t *dst, uint32_t bytes) {
	const uint32_t rate = 48000, freq = 440;
	const uint32_t period = rate / freq;       /* frames/cycle (~109) */
	const uint32_t halfperiod = period / 2;
	int16_t *pcm = (int16_t *)dst;
	const uint32_t frames = bytes / 4;         /* 4 bytes/frame (S16LE stereo) */
	const int16_t amp = 6000;
	for (uint32_t i = 0; i < frames; i++) {
		int16_t s = ((i % period) < halfperiod) ? amp : (int16_t)-amp;
		pcm[i*2 + 0] = s;
		pcm[i*2 + 1] = s;
	}
}

static bool ac97_reset_and_unmute(void) {
	/* Exit cold reset (run): set bit1 in GLOB_CNT. */
	outl(g.nabm + AC97_NABM_GLOB_CNT, 0x00000002);

	/* Soft reset codec registers (write-anything) */
	outw(g.nam + AC97_NAM_RESET, 0x0000);

	/* Unmute + max out */
	outw(g.nam + AC97_NAM_MASTER_VOL,  0x0000);
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

static bool ac97_build_one_shot(void) {
	/* ~200 ms @ 48 kHz stereo S16LE */
	const uint32_t ms = 200;
	const uint32_t frames = (48000u * ms) / 1000u;   /* 9600 frames */
	const uint32_t bytes  = frames * 4u;             /* 38400 bytes */

	uint8_t *buf = (uint8_t *)kmalloc_low(bytes + 16);
	struct ac97_bdl_entry *bdl = (struct ac97_bdl_entry *)kmalloc_low(sizeof(*bdl) + 16);
	if (!buf || !bdl) return false;

	memset(buf, 0, bytes);
	fill_square_440(buf, bytes);

	g.buf       = (uint8_t *)(((uintptr_t)buf + 15) & ~(uintptr_t)15);
	g.buf_phys  = (uint32_t)(uintptr_t)g.buf;
	g.buf_bytes = bytes;

	g.bdl       = (struct ac97_bdl_entry *)(((uintptr_t)bdl + 15) & ~(uintptr_t)15);
	g.bdl_phys  = (uint32_t)(uintptr_t)g.bdl;

	/* Count is in SAMPLES (all channels), not bytes: 2 * frames */
	g.bdl[0].addr  = g.buf_phys;
	g.bdl[0].count = (uint16_t)(frames * 2u);
	g.bdl[0].flags = (1u << 14); /* BUP set so engine stops after this entry */

	return true;
}

static void ac97_start_po(void) {
	/* Reset PCM-OUT box */
	outb(g.nabm + AC97_NABM_PO_CR, AC97_CR_RST);
	for (int i = 0; i < 1000 && (inb(g.nabm + AC97_NABM_PO_CR) & AC97_CR_RST); i++)
		delay_us(10);

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

	g.nam  = (uint16_t)pci_io_base(bar0);
	g.nabm = (uint16_t)pci_io_base(bar1);

	pci_bus_master(dev);

	if (!ac97_reset_and_unmute()) return false;
	if (!ac97_build_one_shot()) {
		dprintf("ac97: buffer alloc failed\n");
		return false;
	}

	ac97_start_po();

	/* Tiny visibility */
	uint8_t civ   = inb(g.nabm + AC97_NABM_PO_CIV);
	uint8_t lvi   = inb(g.nabm + AC97_NABM_PO_LVI);
	uint16_t picb = inw(g.nabm + AC97_NABM_PO_PICB);
	dprintf("ac97: NAM=0x%04x NABM=0x%04x | CIV=%u LVI=%u PICB=%u(samples)\n",
		g.nam, g.nabm, civ, lvi, picb);

	return true;
}

static void init_ac97(void) {
	/* Find by classcode 0x0401 (like your AHCI example uses 0x0106) */
	pci_dev_t dev = pci_get_device(0, 0, 0x0401);
	if (!dev.bits) {
		dprintf("ac97: no devices (class 0x0401)\n");
		return;
	}
	if (ac97_start(dev)) {
		kprintf("ac97: started (one-shot beep)\n");
	} else {
		dprintf("ac97: start failed\n");
	}
}

/* ===================== Module entry points ===================== */
bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("ac97: module loaded\n");
	init_ac97();
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	/* For a one-shot demo we don’t tear down; engine stops on its own */
	dprintf("ac97: module unloaded\n");
	return false;
}
