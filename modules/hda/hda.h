#pragma once
#include <stdint.h>
#include <stdbool.h>

#define HDA_PCI_CLASSC 0x0403

/* Controller MMIO registers (offsets from BAR0) */
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

/* Widget types (P_AW_CAPS bits 23:20) — per HDA spec */
#define WTYPE_AUDIO_OUT     0x00
#define WTYPE_AUDIO_IN      0x01
#define WTYPE_MIXER         0x02
#define WTYPE_SELECTOR      0x03
#define WTYPE_PIN_COMPLEX   0x04
/* (others exist but aren’t needed here: POWER=0x05, VOL_KNOB=0x06, BEEP=0x07, VENDOR=0x0F) */

/* Pin caps bits */
#define PINCAP_OUT          (1u << 4)

/* Default config decode (spec): 31:30 connectivity, 29:24 location, 23:20 default device, 19:16 conn type */
#define DEFDEV_SPEAKER      0x01
#define DEFDEV_LINEOUT      0x02
#define DEFDEV_HP_OUT       0x0F

/* SDnFMT fields (spec 3.3.41) */
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

/* Driver state */
typedef struct {
	uint32_t mmio;              /* BAR0 MMIO base (mapped) */
	/* CORB/RIRB rings (phys + virt) */
	uint64_t *corb;
	uint64_t *rirb;
	uint32_t corb_phys;
	uint32_t rirb_phys;
	uint16_t corb_entries;      /* 256 */
	uint16_t rirb_entries;      /* 256 */

	/* Chosen codec/function/wid path */
	uint8_t cad;               /* codec address (0..15) */
	uint8_t afg_nid;           /* audio function group node id */
	uint8_t dac_nid;           /* output converter (DAC) NID */
	uint8_t pin_nid;           /* output pin NID */

	/* Single output stream descriptor we use (first HW output stream) */
	uint8_t out_sd_index;      /* 0-based among output SDs */
	uint8_t out_stream_tag;    /* 1..15; we use 1 */

	/* BDL / audio ring */
	hda_bdl_entry_t *bdl;
	uint32_t bdl_phys;
	uint8_t *buf;
	uint32_t buf_phys;
	uint32_t bdl_n;         /* 32 */
	uint32_t frag_bytes;    /* fragment size in bytes */
	uint32_t frag_frames;   /* frames per fragment (S16LE stereo => 4 bytes/frame) */
	int tail;          /* last valid index in the ring (−1 if empty) */
	bool started;
	uint32_t rate_hz;       /* 44100 */

} hda_dev_t;

/* Codec topology discovery (pick DAC + output Pin) */
typedef struct {
	uint8_t start_nid;
	uint8_t count;
} nid_range_t;

