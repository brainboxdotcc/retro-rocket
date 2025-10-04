#pragma once
#include <kernel.h>
#include "usb_core.h"

struct usb_ep {
	uint8_t epid;          /* xHCI EP ID (1 = EP0, 2/3 = EP1 OUT/IN, etc.) */
	uint16_t mps;          /* max packet size */
	uint8_t type;          /* 0=ctrl,1=iso,2=bulk,3=int */
	uint8_t dir_in;        /* 1=in, 0=out (for non-ctrl) */
};

/* Probe + bring-up a single xHCI controller on a given PCI function. */
int xhci_probe_and_init(uint8_t bus, uint8_t dev, uint8_t func);

/* Submit a control transfer on EP0.
   setup must be 8 bytes (bmRequestType,bRequest,wValue,wIndex,wLength).
   If data_dir_in=1, data points to IN buffer; else it points to OUT buffer.
   Returns 0 on success; negative on error. */
int xhci_ctrl_xfer(struct usb_dev *ud, const uint8_t *setup,
		   void *data, uint16_t len, int data_dir_in);

/* Arm a persistent interrupt-IN transfer for HID (8-byte reports typical).
   The buffer must be DMA-safe and 64-byte aligned. On each completion,
   the host driver calls the provided callback with the filled data. */
typedef void (*xhci_int_in_cb)(struct usb_dev *ud, const uint8_t *pkt, uint16_t len);
int xhci_arm_int_in(struct usb_dev *ud, uint16_t pkt_len, xhci_int_in_cb cb);

void init_usb_xhci(void);

/* ============================================================
 * xHCI — interrupt-driven (legacy INTx), single-controller v0
 * HID class runs without polling; commands are polled for v0.
 * Uses get_ticks() (ms since boot) for timing.
 * ============================================================ */

/* ---- registers & constants (subset) ---- */
#define XHCI_CAPLENGTH      0x00
#define XHCI_HCSPARAMS1     0x04
#define XHCI_HCSPARAMS2     0x08
#define XHCI_HCCPARAMS1     0x10
#define XHCI_DBOFF          0x14
#define XHCI_RTSOFF         0x18

/* operational (base = cap + caplength) */
#define XHCI_USBCMD         0x00
#define   USBCMD_RS         (1u << 0)
#define   USBCMD_HCRST      (1u << 1)
#define   USBCMD_INTE       (1u << 2)
#define XHCI_USBSTS         0x04
#define   USBSTS_HCH        (1u << 0)
#define   USBSTS_EINT       (1u << 3)
#define   USBSTS_CNR        (1u << 11)
#define XHCI_CRCR           0x18
#define XHCI_DCBAP          0x30
#define XHCI_CONFIG         0x38
#define XHCI_PORTREG_BASE   0x400
#define XHCI_PORT_STRIDE    0x10
#define  PORTSC             0x00
#define    PORTSC_CCS       (1u << 0)
#define    PORTSC_PR        (1u << 4)
#define    PORTSC_WRC       (1u << 19)

/* doorbells (db + 4*target) */
#define XHCI_DOORBELL(off)  (off)

/* runtime interrupter regs (per-interrupter, 32 bytes) */
#define XHCI_RT_IR0         0x20
#define IR_IMAN             0x00
#define   IR_IMAN_IP        (1u << 0)
#define   IR_IMAN_IE        (1u << 1)
#define IR_IMOD             0x04
#define IR_ERSTSZ           0x08
#define IR_ERSTBA           0x10
#define IR_ERDP             0x18

/* TRB types */
#define TRB_NORMAL          1
#define TRB_SETUP_STAGE     2
#define TRB_DATA_STAGE      3
#define TRB_STATUS_STAGE    4
#define TRB_LINK            6
#define TRB_EVENT_DATA      7
#define TRB_NOOP            8
#define TRB_ENABLE_SLOT     9
#define TRB_DISABLE_SLOT    10
#define TRB_ADDRESS_DEVICE  11
#define TRB_CONFIGURE_EP    12
#define TRB_EVAL_CONTEXT    13
#define TRB_TRANSFER_EVENT  32
#define TRB_CMD_COMPLETION  33
#define TRB_PORT_STATUS     34

/* endpoint ids per xHCI: 1=EP0, 2/3=EP1 OUT/IN, ... */
#define EPID_CTRL           1
#define EPID_EP1_IN         3

/* bits in TRB dword3 */
#define TRB_CYCLE           (1u << 0)
#define TRB_ENT             (1u << 1)
#define TRB_ISP             (1u << 2)
#define TRB_NS              (1u << 3)
#define TRB_CH              (1u << 4)
#define TRB_IOC             (1u << 5)
#define TRB_IDT             (1u << 6)
#define TRB_DIR             (1u << 16)

#define XHCI_DNCTRL 0x14

/* helper macros to build TRB fields */
#define TRB_SET_TYPE(x)     ((uint32_t)((x) & 0x3Fu) << 10)

/* ---- minimal structures ---- */
struct trb {
	uint32_t lo;
	uint32_t hi;
	uint32_t sts;    /* len/flags/context-dependent */
	uint32_t ctrl;   /* type/cycle/etc. */
} __attribute__((packed, aligned(16)));

struct erst_entry {
	uint64_t ring_base;
	uint32_t size;
	uint32_t rsvd;
} __attribute__((packed, aligned(16)));

struct xhci_ring {
	struct trb *base;
	uint64_t    phys;
	uint32_t    num_trbs;   /* 256 for a 4K page */
	uint32_t    enqueue;    /* index */
	uint32_t    cycle;      /* 0/1 */
};

struct ep_ctx {
	uint32_t dword0;  /* mps/burst/type/err */
	uint32_t dword1;
	uint64_t deq;     /* dequeue pointer (DCS in bit0) */
	uint32_t dword4;  /* avg TRB len etc. */
	uint32_t rsvd[3];
};

struct slot_ctx {
	uint32_t dword0;  /* speed/context entries */
	uint32_t dword1;  /* root port */
	uint32_t dword2;
	uint32_t dword3;  /* addr/state */
	uint32_t rsvd[4];
};

struct input_ctx {
	uint32_t drop_flags;
	uint32_t add_flags;
	uint32_t rsvd[6];
	struct slot_ctx slot;
	struct ep_ctx   ep0;
	struct ep_ctx   ep1_out;
	struct ep_ctx   ep1_in;
} __attribute__((packed, aligned(64)));

struct xhci_hc {
	/* mmio bases */
	volatile uint8_t  *cap;
	volatile uint8_t  *op;
	volatile uint8_t  *db;
	volatile uint8_t  *rt;

	/* parameters */
	uint8_t cap_len;
	uint8_t max_slots;
	uint8_t csz64; /* 1 if 64B contexts; we use 32B for v0 */
	uint8_t port_count;

	/* rings */
	struct xhci_ring cmd;
	struct xhci_ring evt;

	/* event ring table (single segment) */
	struct erst_entry *erst;
	uint64_t erst_phys;

	/* dcbaa + scratchpad */
	uint64_t *dcbaa;        /* 256 entries */
	uint64_t  dcbaa_phys;
	uint64_t *scratch_index;/* array of ptrs to pads */
	uint64_t  scratch_index_phys;

	/* per-device (single device for v0) */
	struct {
		struct input_ctx *ic;
		uint64_t          ic_phys;
		uint8_t           slot_id;
		uint8_t           dev_addr;

		struct xhci_ring  ep0_tr;
		struct xhci_ring  int_in_tr;

		/* callback for INT-IN */
		xhci_int_in_cb    int_cb;
		uint16_t          int_pkt_len;
		uint8_t          *int_buf;         /* dma buffer for reports */
		uint64_t          int_buf_phys;
	} dev;

	/* irq bookkeeping */
	uint8_t irq_line;
	uint8_t irq_pin;
};

