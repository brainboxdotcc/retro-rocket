/**
 * @file modules/hda/hda.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 *
 * Intel High Definition Audio (Azalia) output driver
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

/** PCI class code for Intel High Definition Audio controllers. */
#define HDA_PCI_CLASSC 0x0403

/* Controller MMIO registers (offsets from BAR0) */
#define HDA_REG_GCAP      0x00 /**< Global Capabilities register. */
#define HDA_REG_VMIN      0x02 /**< Minor version. */
#define HDA_REG_VMAJ      0x03 /**< Major version. */
#define HDA_REG_GCTL      0x08 /**< Global Control; bit0 = CRST (controller reset). */
#define HDA_REG_WAKEEN    0x0C /**< Wake enable. */
#define HDA_REG_STATESTS  0x0E /**< Codec presence status bits (0..2). */
#define HDA_REG_INTCTL    0x20 /**< Global interrupt control. */
#define HDA_REG_INTSTS    0x24 /**< Global interrupt status (R/WC). */
#define HDA_REG_WALCLK    0x30 /**< Free-running wall clock (~24.576 MHz). */
#define HDA_REG_SSYNC     0x34 /**< Stream synchronisation. */

/* CORB / RIRB registers */
#define HDA_REG_CORBLBASE 0x40 /**< CORB lower base address. */
#define HDA_REG_CORBUBASE 0x44 /**< CORB upper base address. */
#define HDA_REG_CORBWP    0x48 /**< CORB write pointer. */
#define HDA_REG_CORBRP    0x4A /**< CORB read pointer. */
#define HDA_CORBRP_RST   0x8000 /**< Reset CORB read pointer. */
#define HDA_REG_CORBCTL   0x4C /**< CORB control. */
#define HDA_CORBCTL_MEIE 0x01  /**< CORB memory error interrupt enable. */
#define HDA_CORBCTL_DMAE 0x02  /**< CORB DMA enable. */
#define HDA_REG_CORBSTS   0x4D /**< CORB status. */
#define HDA_REG_CORBSIZE  0x4E /**< CORB size; bits[1:0]=00:2,01:16,10:256 entries. */

#define HDA_REG_RIRBLBASE 0x50 /**< RIRB lower base address. */
#define HDA_REG_RIRBUBASE 0x54 /**< RIRB upper base address. */
#define HDA_REG_RIRBWP    0x58 /**< RIRB write pointer. */
#define HDA_RIRBWP_RST   0x8000 /**< Reset RIRB write pointer. */
#define HDA_REG_RINTCNT   0x5A /**< RIRB response interrupt count. */
#define HDA_REG_RIRBCTL   0x5C /**< RIRB control. */
#define HDA_RIRBCTL_INT_ON_OVERRUN 0x01 /**< Interrupt on overrun. */
#define HDA_RIRBCTL_DMAE 0x02 /**< RIRB DMA enable. */
#define HDA_RIRBCTL_IRQE 0x04 /**< RIRB interrupt enable. */
#define HDA_REG_RIRBSTS   0x5D /**< RIRB status. */
#define HDA_REG_RIRBSIZE  0x5E /**< RIRB size; encoding matches CORBSIZE. */

/* DMA position buffer registers (not used here, but useful for debugging latency) */
#define HDA_REG_DPLBASE   0x70 /**< DMA position buffer lower base. */
#define HDA_REG_DPUBASE   0x74 /**< DMA position buffer upper base. */

/* Stream descriptor block layout (SDn) */
#define HDA_SD_BASE       0x80 /**< Base offset of first stream descriptor. */
#define HDA_SD_STRIDE     0x20 /**< Stride between SDn blocks. */

/* Offsets inside SDn */
#define SD_CTL0    0x00 /**< Control0: RUN, SRST, tag/prio. */
#define SD_CTL2    0x02 /**< Control2: stream tag in [7:4]. */
#define SD_STS     0x03 /**< Status (R/WC). */
#define SD_LPIB    0x04 /**< Link position in bytes. */
#define SD_CBL     0x08 /**< Cyclic buffer length in bytes. */
#define SD_LVI     0x0C /**< Last valid index. */
#define SD_FMT     0x12 /**< Stream format. */
#define SD_BDPL    0x18 /**< BDL pointer low. */
#define SD_BDPU    0x1C /**< BDL pointer upper. */

/* SD_CTL0 bits */
#define SD_CTL_RUN  (1 << 1) /**< Start/stop stream DMA. */
#define SD_CTL_SRST (1 << 0) /**< Stream reset. */

/**
 * Buffer Descriptor List entry (HDA).
 * Each entry is 16 bytes, aligned to 16.
 */
typedef struct {
	uint64_t addr;  /**< Physical address of buffer. */
	uint32_t len;   /**< Length in bytes. */
	uint32_t flags; /**< Flags; bit0 = IOC (interrupt on completion). */
} __attribute__((packed)) hda_bdl_entry_t;

/**
 * Construct a verb value for the CORB or Immediate Command registers.
 * Fields: [31:28] cad, [27:20] nid, [19:8] verb, [7:0] payload.
 */
#define HDA_MAKE_VERB(cad, nid, verb, payload) \
    ( ((uint32_t)(cad) & 0xF) << 28 | ((uint32_t)(nid) & 0x7F) << 20 | ((uint32_t)(verb) & 0xFFF) << 8 | ((uint32_t)(payload) & 0xFF) )

/* Common verbs */
#define V_GET_PARAM         0xF00 /**< Get parameter. */
#define V_GET_CONN_LIST     0xF02 /**< Get connection list. */
#define V_SET_POWER_STATE   0x705 /**< Set power state; 0 = D0. */
#define V_SET_CONV_STREAMCH 0x706 /**< Assign stream/channel. */
#define V_SET_PIN_WCTRL     0x707 /**< Set pin widget control. */
#define V_GET_PIN_SENSE     0xF09 /**< Get pin sense. */
#define V_SET_EAPD_BTL      0x70C /**< Set EAPD/BTL enable. */
#define V_GET_DEF_CFG       0xF1C /**< Get default configuration. */
#define V_SET_CHANNEL_COUNT 0x72D /**< Set channel count. */
#define V_SET_AMP_GAIN_MUTE 0x003 /**< Set amplifier gain/mute. */

/* GET_PARAM indices */
#define P_NODE_COUNT        0x04 /**< Node count (high=start, low=count). */
#define P_FUNC_GRP_TYPE     0x05 /**< Function group type; 1=AFG. */
#define P_AW_CAPS           0x09 /**< Audio widget capabilities. */
#define P_PIN_CAPS          0x0C /**< Pin widget capabilities. */

/* Widget types (from P_AW_CAPS bits 23:20) */
#define WTYPE_AUDIO_OUT     0x00 /**< Audio output converter. */
#define WTYPE_AUDIO_IN      0x01 /**< Audio input converter. */
#define WTYPE_MIXER         0x02 /**< Mixer. */
#define WTYPE_SELECTOR      0x03 /**< Selector. */
#define WTYPE_PIN_COMPLEX   0x04 /**< Pin complex. */

/* Pin caps bits */
#define PINCAP_OUT          (1 << 4) /**< Pin supports output. */

/* Default configuration decode values (bits 23:20 default device) */
#define DEFDEV_SPEAKER      0x01 /**< Default device: speaker. */
#define DEFDEV_LINEOUT      0x02 /**< Default device: line-out. */
#define DEFDEV_HP_OUT       0x0F /**< Default device: headphone. */

/* SDnFMT field helpers */
#define SD_FMT_BASE_48K     (0 << 14) /**< Base rate 48 kHz. */
#define SD_FMT_BASE_44K1    (1 << 14) /**< Base rate 44.1 kHz. */
#define SD_FMT_MULT_X1      (0 << 11) /**< Multiplier ×1. */
#define SD_FMT_MULT_X2      (1 << 11) /**< Multiplier ×2. */
#define SD_FMT_DIV_1        (0 << 8)  /**< Divider ÷1. */
#define SD_FMT_DIV_2        (1 << 8)  /**< Divider ÷2. */
#define SD_FMT_BITS_16      (1 << 4)  /**< 16-bit samples. */
#define SD_FMT_CHANS(n)     ((uint16_t)(((n) - 1u) & 0x0F)) /**< Channels = n. */

/**
 * Driver state for HDA controller and single output stream.
 */
typedef struct {
	uint32_t mmio;              /**< BAR0 MMIO base (mapped). */

	/* CORB / RIRB rings */
	uint64_t *corb;             /**< CORB virtual base. */
	uint64_t *rirb;             /**< RIRB virtual base. */
	uint32_t corb_phys;         /**< CORB physical base. */
	uint32_t rirb_phys;         /**< RIRB physical base. */
	uint16_t corb_entries;      /**< CORB entries (typically 256). */
	uint16_t rirb_entries;      /**< RIRB entries (typically 256). */

	/* Chosen codec/function/path */
	uint8_t cad;                /**< Codec address (0..15). */
	uint8_t afg_nid;            /**< Audio function group node id. */
	uint8_t dac_nid;            /**< DAC node id. */
	uint8_t pin_nid;            /**< Pin node id. */

	/* Output stream descriptor */
	uint8_t out_sd_index;       /**< Index of output stream descriptor. */
	uint8_t out_stream_tag;     /**< Stream tag (1..15). */

	/* Buffer descriptor list (BDL) and audio ring buffer */
	hda_bdl_entry_t *bdl;       /**< BDL virtual base. */
	uint32_t bdl_phys;          /**< BDL physical base. */
	uint8_t *buf;               /**< Audio buffer virtual base. */
	uint32_t buf_phys;          /**< Audio buffer physical base. */
	uint32_t bdl_n;             /**< Number of BDL entries. */
	uint32_t frag_bytes;        /**< Fragment size in bytes. */
	uint32_t frag_frames;       /**< Frames per fragment (4 bytes per stereo S16 frame). */
	int tail;                   /**< Last valid BDL index; −1 if empty. */
	bool started;               /**< Stream running state. */
	uint32_t rate_hz;           /**< Current sample rate. */
} hda_dev_t;

/**
 * Simple node range (start NID, count).
 * Used during codec topology discovery.
 */
typedef struct {
	uint8_t start_nid; /**< Starting node id. */
	uint8_t count;     /**< Number of nodes. */
} nid_range_t;
