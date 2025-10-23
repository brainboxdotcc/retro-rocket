#pragma once

#define VIRTIO_VENDOR_ID          0x1AF4
#define VIRTIO_DEVICE_BLOCK_MODERN  0x1042

#define VIRTIO_PCI_CAP_COMMON_CFG  1
#define VIRTIO_PCI_CAP_NOTIFY_CFG  2
#define VIRTIO_PCI_CAP_ISR_CFG     3
#define VIRTIO_PCI_CAP_DEVICE_CFG  4

typedef struct virtio_pci_common_cfg_t {
	uint32_t device_feature_select;   /* 0x00 */
	uint32_t device_feature;          /* 0x04 */
	uint32_t driver_feature_select;   /* 0x08 */
	uint32_t driver_feature;          /* 0x0C */
	uint16_t msix_config;             /* 0x10 */
	uint16_t num_queues;              /* 0x12 */
	uint8_t device_status;           /* 0x14 */
	uint8_t config_generation;       /* 0x15 */

	/* About the currently selected queue: */
	uint16_t queue_select;            /* 0x16 */
	uint16_t queue_size;              /* 0x18 */
	uint16_t queue_msix_vector;       /* 0x1A */
	uint16_t queue_enable;            /* 0x1C */
	uint16_t queue_notify_off;        /* 0x1E */
	uint64_t queue_desc;              /* 0x20 */
	uint64_t queue_driver;            /* 0x28 */
	uint64_t queue_device;            /* 0x30 */
} __attribute__((packed)) virtio_pci_common_cfg_t;

/* ===== Virtio device-specific config (block) ===== */
typedef struct virtio_block_config_t {
	uint64_t capacity;       /* in 512-byte sectors */
	uint32_t size_max;
	uint32_t seg_max;
	struct {
		uint16_t cylinders;
		uint8_t heads;
		uint8_t sectors;
	} geometry;
	uint32_t block_size;
	uint32_t physical_block_exp;
	uint32_t alignment_offset;
	uint32_t min_io_size;
	uint32_t opt_io_size;
	uint8_t writeback;          /* deprecated, ignore */
	uint8_t unused1[3];
	uint32_t max_discard_sectors;
	uint32_t max_discard_seg;
	uint32_t discard_sector_alignment;
	uint32_t max_write_zeroes_sectors;
	uint32_t max_write_zeroes_seg;
	uint8_t write_zeroes_may_unmap;
	uint8_t unused2[3];
	uint32_t max_secure_erase_sectors;
	uint32_t max_secure_erase_seg;
	uint32_t secure_erase_sector_alignment;
} __attribute__((packed)) virtio_block_config_t;

/* ===== Virtqueue data structures ===== */
#define VIRTQ_DESC_F_NEXT   1
#define VIRTQ_DESC_F_WRITE  2

typedef struct virtq_desc_t {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
} __attribute__((packed)) virtq_desc_t;

typedef struct virtq_avail_t {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[]; /* size = queue_size */
} __attribute__((packed)) virtq_avail_t;

typedef struct virtq_used_elem_t {
	uint32_t id;   /* index of start desc */
	uint32_t len;  /* bytes written by device */
} __attribute__((packed)) virtq_used_elem_t;

typedef struct virtq_used_t {
	uint16_t flags;
	uint16_t idx;
	virtq_used_elem_t ring[]; /* size = queue_size */
} __attribute__((packed)) virtq_used_t;

/* ===== Virtio-block request header/footer ===== */
#define VIRTIO_BLOCK_T_IN     0
#define VIRTIO_BLOCK_T_OUT    1

typedef struct virtio_block_req_hdr_t {
	uint32_t type;      /* IN=0, OUT=1 */
	uint32_t reserved;
	uint64_t sector;    /* 512B sector units */
} __attribute__((packed)) virtio_block_req_hdr_t;

typedef struct virtio_block_req_status_t {
	uint8_t status;     /* 0 = OK */
} __attribute__((packed)) virtio_block_req_status_t;

/* ===== Storage device wiring ===== */
typedef struct virtio_block_dev_t {
	/* MMIO window */
	volatile uint8_t *bar_base;
	uint64_t bar_size;

	/* Virtio mapped sub-windows (from PCI caps) */
	volatile virtio_pci_common_cfg_t *common;
	volatile uint8_t *notify_base;
	uint32_t notify_off_mul;
	volatile uint8_t *isr;
	volatile virtio_block_config_t *device_cfg;

	/* One queue (queue 0) */
	uint16_t q_size;   /* power-of-two (device-provided) */
	uint16_t q_mask;

	/* Rings (single queue) */
	virtq_desc_t *desc;
	virtq_avail_t *avail;
	virtq_used_t *used;

	/* Physical (identity == virtual) for PRP-like addressing */
	uint64_t desc_phys;
	uint64_t avail_phys;
	uint64_t used_phys;

	/* Descriptor state (single request at a time) */
	uint16_t free_head; /* start of free list (we link sequentially) */
	uint16_t used_last; /* last seen used idx */

	/* Scratch buffers for request header/footer */
	virtio_block_req_hdr_t *hdr;
	virtio_block_req_status_t *st;

	/* Device properties */
	uint64_t capacity_512;  /* from device_cfg->capacity */
	uint32_t logical_block; /* 512 or device_cfg->block_size if non-zero */

} virtio_block_dev_t;

/* Capability header (per virtio-pci modern) */
typedef struct virtio_pci_cap_hdr_t {
	uint8_t cap_vndr;     /* 0x09 for vendor-specific */
	uint8_t cap_next;
	uint8_t cap_len;
	uint8_t cfg_type;     /* VIRTIO_PCI_CAP_* */
	uint8_t bar;          /* which BAR */
	uint8_t padding[3];
	uint32_t offset;       /* within BAR */
	uint32_t length;
} __attribute__((packed)) virtio_pci_cap_hdr_t;

typedef struct virtio_pci_notify_cap_t {
	virtio_pci_cap_hdr_t cap;
	uint32_t notify_off_multiplier;
} __attribute__((packed)) virtio_pci_notify_cap_t;

#define VIRTIO_STATUS_ACKNOWLEDGE  (1 << 0)
#define VIRTIO_STATUS_DRIVER       (1 << 1)
#define VIRTIO_STATUS_DRIVER_OK    (1 << 2)
#define VIRTIO_STATUS_FEATURES_OK  (1 << 3)
#define VIRTIO_STATUS_FAILED       (1 << 7)

void init_virtio_block(void);