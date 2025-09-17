#pragma once
#include <kernel.h>
#include <stdbool.h>

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

struct ac97_bdl_entry {
	uint32_t addr;   /* phys addr of audio data */
	uint16_t count;  /* number of samples in this buffer (all channels) */
	uint16_t flags;  /* bit15=IOC, bit14=BUP (buffer underrun policy / stop) */
} __attribute__((packed));

typedef struct {
	uint16_t nam;          /* I/O base (NAM) */
	uint16_t nabm;         /* I/O base (NABM) */
	struct ac97_bdl_entry *bdl;
	uint32_t bdl_phys;
	uint8_t *buf;
	uint32_t buf_phys;
	uint32_t buf_bytes;
	uint8_t bdl_n;         /* always 32 (hardware modulo) */
	uint32_t frag_bytes;    /* bytes per descriptor (multiple of 128, multiple of 4) */
	uint32_t frag_frames;   /* frag_bytes / 4 (S16LE stereo) */
	int tail;          /* last valid index we’ve published to HW; -1 means empty */
	bool started;       /* PO engine running flag */

} ac97_dev_t;
