#pragma once

#include <kernel.h>
#include "virtio.h"

/**
 * @def VIRTIO_DEVICE_NET_MODERN
 * @brief PCI device ID for modern (non-transitional) virtio-net.
 *
 * Transitional virtio-net typically reports 0x1000; modern devices use 0x1041.
 */
#define VIRTIO_DEVICE_NET_MODERN 0x1041

/**
 * @def VIRTIO_PCI_CAP_COMMON_CFG
 * @brief Vendor capability: common configuration MMIO window selector.
 */
#define VIRTIO_PCI_CAP_COMMON_CFG  1
/**
 * @def VIRTIO_PCI_CAP_NOTIFY_CFG
 * @brief Vendor capability: notify (doorbell) MMIO window selector.
 */
#define VIRTIO_PCI_CAP_NOTIFY_CFG  2
/**
 * @def VIRTIO_PCI_CAP_ISR_CFG
 * @brief Vendor capability: ISR byte MMIO window selector (legacy/INTx paths).
 */
#define VIRTIO_PCI_CAP_ISR_CFG     3
/**
 * @def VIRTIO_PCI_CAP_DEVICE_CFG
 * @brief Vendor capability: device-specific configuration MMIO window selector.
 */
#define VIRTIO_PCI_CAP_DEVICE_CFG  4

/**
 * @def VIRTIO_F_VERSION_1
 * @brief Feature bit indicating the device speaks modern virtio 1.x.
 *
 * This lives in the upper 32-bit feature word; keep the constant 64-bit.
 */
#define VIRTIO_F_VERSION_1         (1ULL << 32)
/**
 * @def VIRTIO_NET_F_MAC
 * @brief Feature bit: device exposes a fixed MAC address in its config space.
 */
#define VIRTIO_NET_F_MAC           (1 << 5)

/**
 * @def VIRTQ_DESC_F_NEXT
 * @brief Virtqueue descriptor flag: this entry chains to `next`.
 */
#define VIRTQ_DESC_F_NEXT          1
/**
 * @def VIRTQ_DESC_F_WRITE
 * @brief Virtqueue descriptor flag: device may write to this buffer.
 */
#define VIRTQ_DESC_F_WRITE         2

/**
 * @def VNET_QSIZE
 * @brief Driver’s requested queue size (clamped to device’s `queue_size`).
 */
#define VNET_QSIZE                 256
/**
 * @def VNET_RX_BUF_SIZE
 * @brief Size of each posted RX buffer (must cover virtio header + frame).
 */
#define VNET_RX_BUF_SIZE           2048
/**
 * @def VNET_HDR_SIZE
 * @brief Size of the mandatory virtio-net header prepended to each packet.
 */
#define VNET_HDR_SIZE              (sizeof(virtio_net_hdr_t))

/**
 * @brief Device-specific configuration for virtio-net.
 *
 * Only the fixed MAC and a status word are consumed by this driver.
 */
typedef struct {
	/** Station MAC address provided by the device when @ref VIRTIO_NET_F_MAC is set */
	uint8_t mac[6];
	/** Link/status flags (if implemented by the device; not required by this driver) */
	uint16_t status;
} __attribute__((packed)) virtio_net_config_t;

/**
 * @brief Minimal virtio-net per-packet header (no offloads negotiated).
 *
 * This header precedes every TX and RX payload. All fields are little-endian.
 * The driver zero-initialises it; the device may fill some fields on RX when
 * certain features are negotiated (not used here).
 */
typedef struct {
	/** Flags (eg. checksum present); unused in this minimal driver */
	uint8_t  flags;
	/** GSO type (eg. TCPv4); unused in this minimal driver */
	uint8_t  gso_type;
	/** Length of this header if larger via extensions; unused here */
	uint16_t hdr_len;
	/** GSO segment size; unused here */
	uint16_t gso_size;
	/** Checksum start offset; unused here */
	uint16_t csum_start;
	/** Checksum offset from @ref csum_start; unused here */
	uint16_t csum_offset;
	/** Number of buffers (mergeable RX); unused here */
	uint16_t num_buffers;
} __attribute__((packed)) virtio_net_hdr_t;

/**
 * @brief One-byte ISR window (legacy/INTx ack by read-to-clear).
 */
typedef struct {
	/** Interrupt cause byte; reading acknowledges in legacy paths */
	volatile uint8_t isr;
} virtio_pci_isr_t;

/**
 * @brief In-memory representation of a single virtqueue.
 */
typedef struct virtq {
	/** Queue size (power-of-two), negotiated from device `queue_size` */
	uint16_t q_size;
	/** Descriptor table base (guest-virtual; identity-mapped to GPA) */
	virtq_desc_t *desc;
	/** Driver “avail” ring base */
	virtq_avail_t *avail;
	/** Device “used” ring base */
	virtq_used_t *used;
	/** Head of the free-list of descriptors (0xFFFF = empty) */
	uint16_t free_head;
	/** Driver shadow of `avail->idx` (monotonic) */
	uint16_t avail_idx;
	/** Driver shadow of `used->idx` last consumed (monotonic) */
	uint16_t used_idx;
	/** Per-queue notify offset (read from common config after select) */
	uint16_t notify_off;
} virtq_t;

/**
 * @brief Driver static state for a single virtio-net instance.
 */
static struct {
	/** Pointer to the common configuration MMIO window (from caps) */
	volatile virtio_pci_common_cfg_t *common;
	/** Base of the notify (doorbell) MMIO window */
	volatile uint8_t *notify_base;
	/** Optional ISR byte window (legacy/INTx); may be NULL under MSI/MSI-X */
	volatile virtio_pci_isr_t *isr;
	/** Pointer to the device-specific net configuration (MAC, status) */
	volatile virtio_net_config_t *devcfg;
	/** Notify offset multiplier read from the NOTIFY cap body */
	uint32_t notify_off_mul;
	/** Receive queue (queue 0) */
	virtq_t rxq;
	/** Transmit queue (queue 1) */
	virtq_t txq;
	/** Array of RX buffers posted to the device, indexed by descriptor id */
	void *rx_bufs[VNET_QSIZE];
	/** Per-descriptor TX header storage (optional; may be static in .c) */
	void *tx_hdrs[VNET_QSIZE];
	/** Driver’s in-memory copy of the active MAC address */
	uint8_t mac[6];
	/** Cached BAR base pointers by index (identity-mapped) */
	volatile uint8_t *bar_base[6];
	/** Cached BAR sizes by index */
	uint64_t bar_size[6];
	/** PCI device handle bound to this instance */
	pci_dev_t pci;
	/** Cached device ID (lower 16 bits of PCI ID) */
	uint16_t device_id;
} vnet;
