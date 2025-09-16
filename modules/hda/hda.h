#pragma once
#include <kernel.h>
#include <mmio.h>

/* PCI multimedia + HDA subclass */
#define PCI_CLASSCODE_HDA     0x0403u

/* ---- HDA MMIO registers (subset) ---- */
#define HDA_GCAP        0x0000
#define HDA_GCTL        0x0008

/* CORB */
#define HDA_CORBLBASE   0x0040
#define HDA_CORBUBASE   0x0044
#define HDA_CORBWP      0x0048
#define HDA_CORBRP      0x004A
#define HDA_CORBCTL     0x004C
#define HDA_CORBSTS     0x004D
#define HDA_CORBSIZE    0x004E

/* RIRB */
#define HDA_RIRBLBASE   0x0050
#define HDA_RIRBUBASE   0x0054
#define HDA_RIRBWP      0x0058
#define HDA_RINTCNT     0x005A
#define HDA_RIRBCTL     0x005C
#define HDA_RIRBSTS     0x005D
#define HDA_RIRBSIZE    0x005E

/* Output Stream 0 (ICH9 layout) */
#define HDA_SD0CTL      0x0080
#define HDA_SD0STS      0x0083
#define HDA_SD0LPIB     0x0084
#define HDA_SD0CBL      0x0088
#define HDA_SD0LVI      0x008C

/* Bits */
#define GCTL_CRST               0x00000001u
#define CORBCTL_CORBRUN         0x02u
#define RIRBCTL_RIRBRUN         0x02u
#define CORBSIZE_256            0x02u
#define RIRBSIZE_256            0x02u
#define SDCTL_RUN               0x01u
#define SDCTL_SRST              0x02u

/* BDL entry */
struct hda_bdl_entry {
	uint32_t addr_lo;
	uint32_t addr_hi;
	uint32_t length;  /* multiple of 128 bytes */
	uint32_t flags;   /* IOC etc. */
};

/* Verbs (subset) */
#define VERB_GET_PARAMETER          0xF00u
#define VERB_GET_DEFAULT_CFG        0xF1Cu
#define VERB_SET_POWER_STATE        0x705u
#define VERB_SET_PIN_WIDGET_CTRL    0x707u
#define VERB_SET_EAPD_BTL           0x70Cu
#define VERB_SET_CONV_FMT           0x002u
#define VERB_SET_CONV_STREAM_CHAN   0x706u
#define VERB_SET_AMP_GAIN_MUTE      0x003u

/* GET_PARAMETER IDs */
#define PARAM_NODE_COUNT            0x04u
#define PARAM_FUNC_GROUP_TYPE       0x05u
#define PARAM_AUDIO_WIDGET_CAP      0x09u

/* Widget & pin defs */
#define WIDGET_TYPE_PIN             0x04u
#define PIN_DEV_LINE_OUT            0x01u
#define PIN_DEV_SPEAKER             0x02u
#define PIN_DEV_HP_OUT              0x04u

/* --- global regs (width matters) --- */
#define HDA_GCAP        0x0000  /* 16-bit */
#define HDA_GCTL        0x0008  /* 32-bit */
#define HDA_WAKEEN      0x000C  /* 16-bit (optional) */
#define HDA_STATESTS    0x000E  /* 16-bit  <-- was 0x10 (wrong) */
#define HDA_GSTS        0x0010  /* 16/32-bit global status (not used) */

/* --- stream 0 register block (base 0x80, per HDA spec) --- */
#define HDA_SD0CTL      0x0080  /* 8/32-bit */
#define HDA_SD0STS      0x0083  /* 8-bit (W1C) */
#define HDA_SD0LPIB     0x0084  /* 32-bit */
#define HDA_SD0CBL      0x0088  /* 32-bit */
#define HDA_SD0LVI      0x008C  /* 16-bit */
#define HDA_SD0FMT      0x0092  /* 16-bit  <-- was 0xA2 (wrong) */
#define HDA_SD0BDPL     0x0098  /* 32-bit  <-- was 0xA0 (wrong) */
#define HDA_SD0BDPU     0x009C  /* 32-bit  <-- was 0xA4 (wrong) */


/* Build verb */
#define HDA_MAKE_VERB(c, n, v, p) \
    ((((uint32_t)(c)) << 28) | (((uint32_t)(n)) << 20) | (((uint32_t)(v)) << 8) | ((uint32_t)(p)))

/* Public API */
bool   hda_start(pci_dev_t dev);                        /* called after probe */
bool   init_hda(void);                                  /* probe by class 0x0403 and start */
size_t hda_write_frames_48k_s16le(const int16_t *frames, size_t frame_count);
void   hda_test_tone(void);
