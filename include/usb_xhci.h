#pragma once

#include <kernel.h>
#include "usb_core.h"

/**
 * @struct usb_ep
 * @brief Minimal endpoint description used by class drivers and HCD glue
 */
struct usb_ep {
	/** xHCI EP ID (1 = EP0, 2/3 = EP1 OUT/IN, etc.). */
	uint8_t epid;
	/** Maximum packet size. */
	uint16_t mps;
	/** Transfer type (0=ctrl, 1=iso, 2=bulk, 3=int). */
	uint8_t type;
	/** Direction: 1 = IN (device->host), 0 = OUT (host->device). */
	uint8_t dir_in;
};

/** @brief Capability length (capability area size / operational offset). */
#define XHCI_CAPLENGTH      0x00
/** @brief HCSPARAMS1: slots/ports parameters. */
#define XHCI_HCSPARAMS1     0x04
/** @brief HCSPARAMS2: scratchpads and other capabilities. */
#define XHCI_HCSPARAMS2     0x08
/** @brief HCCPARAMS1: structural capability flags. */
#define XHCI_HCCPARAMS1     0x10
/** @brief DBOFF: Doorbell registers offset from CAP base. */
#define XHCI_DBOFF          0x14
/** @brief RTSOFF: Runtime registers offset from CAP base. */
#define XHCI_RTSOFF         0x18

/* ---- operational registers (base = CAP + CAPLENGTH) ---- */

/** @brief USBCMD: controller command register offset. */
#define XHCI_USBCMD         0x00
/** @brief USBCMD.RS: Run/Stop (1=run, 0=halt). */
#define USBCMD_RS           (1u << 0)
/** @brief USBCMD.HCRST: Host Controller Reset. */
#define USBCMD_HCRST        (1u << 1)
/** @brief USBCMD.INTE: Global interrupt enable. */
#define USBCMD_INTE         (1u << 2)

/** @brief USBSTS: controller status register offset. */
#define XHCI_USBSTS         0x04
/** @brief USBSTS.HCH: Host Controller Halted (1 when halted). */
#define USBSTS_HCH          (1u << 0)
/** @brief USBSTS.EINT: Event interrupt pending. */
#define USBSTS_EINT         (1u << 3)
/** @brief USBSTS.CNR: Controller Not Ready after reset. */
#define USBSTS_CNR          (1u << 11)

/** @brief CRCR: Command Ring Control Register offset. */
#define XHCI_CRCR           0x18
/** @brief DCBAP: Device Context Base Address Array pointer offset. */
#define XHCI_DCBAP          0x30
/** @brief CONFIG: Max slots enabled. */
#define XHCI_CONFIG         0x38

/** @brief PORTREG_BASE: First port register block offset. */
#define XHCI_PORTREG_BASE   0x400
/** @brief PORT_STRIDE: Bytes between each port register block. */
#define XHCI_PORT_STRIDE    0x10
/** @brief PORTSC: Port status/control offset within a port block. */
#define PORTSC              0x00
/** @brief PORTSC.CCS: Current Connect Status. */
#define PORTSC_CCS          (1u << 0)
/** @brief PORTSC.PR: Port Reset. */
#define PORTSC_PR           (1u << 4)
/** @brief PORTSC.WRC: Warm Reset Change. */
#define PORTSC_WRC          (1u << 19)

/* ---- doorbells ---- */

/** @brief Computed doorbell register offset (db + 4*target). */
#define XHCI_DOORBELL(off)  (off)

/* ---- runtime interrupter registers (per-interrupter) ---- */

/** @brief Interrupter 0 base offset within runtime regs. */
#define XHCI_RT_IR0         0x20
/** @brief IMAN: Interrupter Management. */
#define IR_IMAN             0x00
/** @brief IMAN.IP: Interrupt Pending (W1C). */
#define IR_IMAN_IP          (1u << 0)
/** @brief IMAN.IE: Interrupter Enable. */
#define IR_IMAN_IE          (1u << 1)
/** @brief IMOD: Interrupter Moderation (interval / counter). */
#define IR_IMOD             0x04
/** @brief ERSTSZ: Event Ring Segment Table Size. */
#define IR_ERSTSZ           0x08
/** @brief ERSTBA: Event Ring Segment Table Base Address. */
#define IR_ERSTBA           0x10
/** @brief ERDP: Event Ring Dequeue Pointer. */
#define IR_ERDP             0x18

/* ---- TRB types ---- */

/** @brief TRB type: Normal (data transfer). */
#define TRB_NORMAL          1
/** @brief TRB type: Setup Stage (control). */
#define TRB_SETUP_STAGE     2
/** @brief TRB type: Data Stage (control). */
#define TRB_DATA_STAGE      3
/** @brief TRB type: Status Stage (control). */
#define TRB_STATUS_STAGE    4
/** @brief TRB type: Link (ring segment link). */
#define TRB_LINK            6
/** @brief TRB type: Event Data (TD delimiter). */
#define TRB_EVENT_DATA      7
/** @brief TRB type: No-Op. */
#define TRB_NOOP            8
/** @brief TRB type: Enable Slot (command). */
#define TRB_ENABLE_SLOT     9
/** @brief TRB type: Disable Slot (command). */
#define TRB_DISABLE_SLOT    10
/** @brief TRB type: Address Device (command). */
#define TRB_ADDRESS_DEVICE  11
/** @brief TRB type: Configure Endpoint (command). */
#define TRB_CONFIGURE_EP    12
/** @brief TRB type: Evaluate Context (command). */
#define TRB_EVAL_CONTEXT    13
/** @brief TRB type: Transfer Event (event). */
#define TRB_TRANSFER_EVENT  32
/** @brief TRB type: Command Completion (event). */
#define TRB_CMD_COMPLETION  33
/** @brief TRB type: Port Status Change (event). */
#define TRB_PORT_STATUS     34
/** @brief TRB type: Bandwidth Request (event). */
#define TRB_BANDWIDTH_REQUEST      35
/** @brief TRB type: Doorbell Event (event). */
#define TRB_DOORBELL_EVENT         36
/** @brief TRB type: Host Controller Event (event). */
#define TRB_HOST_CONTROLLER_EVENT  37
/** @brief TRB type: Device Notification (event). */
#define TRB_DEVICE_NOTIFICATION    38
/** @brief TRB type: Microframe Index Wrap (event). */
#define TRB_MFINDEX_WRAP_EVENT     39

/* ---- endpoint IDs (xHCI numbering) ---- */

/** @brief EPID for default control endpoint (EP0). */
#define EPID_CTRL           1
/** @brief EPID for EP1 IN (interrupt IN for HID boot keyboards). */
#define EPID_EP1_IN         3

/* ---- TRB control bits (dword 3) ---- */

/** @brief Producer/consumer cycle bit. */
#define TRB_CYCLE           (1u << 0)
/** @brief Toggle Cycle on Link TRB. */
#define TRB_TOGGLE          (1u << 1)
/** @brief Evaluate Next TRB (a.k.a. ENT) — alias of TOGGLE per spec bit position. */
#define TRB_ENT             (1u << 1)
/** @brief Interrupt on Short Packet. */
#define TRB_ISP             (1u << 2)
/** @brief No Snoop (PCIe). */
#define TRB_NS              (1u << 3)
/** @brief Chain bit (continue TD). */
#define TRB_CH              (1u << 4)
/** @brief Interrupt on Completion. */
#define TRB_IOC             (1u << 5)
/** @brief Immediate Data (payload in TRB). */
#define TRB_IDT             (1u << 6)
/** @brief Direction: IN (for control/normal as applicable). */
#define TRB_DIR             (1u << 16)

/* ---- PORTSC RW1C change bits ---- */

/** @brief Connect Status Change (write 1 to clear). */
#define PORTSC_CSC   (1u << 17)
/** @brief Port Enable/Disable Change (W1C). */
#define PORTSC_PEC   (1u << 18)
/** @brief Warm Reset Change (W1C). */
#define PORTSC_WRC   (1u << 19)
/** @brief Over-current Change (W1C). */
#define PORTSC_OCC   (1u << 20)
/** @brief Port Reset Change (W1C). */
#define PORTSC_PRC   (1u << 21)
/** @brief Port Link State Change (W1C). */
#define PORTSC_PLC   (1u << 22)
/** @brief Port Config Error Change (W1C). */
#define PORTSC_CEC   (1u << 23)

/* ---- completion codes (subset) ---- */

/** @brief Transfer completed successfully. */
#define CC_SUCCESS        0x01
/** @brief Short packet: transfer completed with fewer bytes than requested. */
#define CC_SHORT_PACKET   0x0D

/* ---- helper masks/macros ---- */

/** @brief Mask of all RW1C change bits in PORTSC. */
#define PORTSC_RW1C_MASK (PORTSC_CSC | PORTSC_PEC | PORTSC_WRC | PORTSC_OCC | PORTSC_PRC | PORTSC_PLC | PORTSC_CEC)
/** @brief Encode TRB type into control dword. */
#define TRB_SET_TYPE(x)     ((uint32_t)((x) & 0x3Fu) << 10)

/**
 * @struct trb
 * @brief Generic 16-byte TRB structure (command, event, or transfer)
 */
struct trb {
	/** Pointer/parameter low dword. */
	uint32_t lo;
	/** Pointer/parameter high dword. */
	uint32_t hi;
	/** Status dword (e.g., transfer length, completion code). */
	uint32_t sts;
	/** Control dword (type, cycle, flags). */
	uint32_t ctrl;
} __attribute__((packed, aligned(16)));

/**
 * @struct erst_entry
 * @brief Single Event Ring Segment Table entry (we use one segment)
 */
struct erst_entry {
	/** Physical address of the event ring segment base. */
	uint64_t ring_base;
	/** Number of TRBs in the segment. */
	uint32_t size;
	/** Reserved. */
	uint32_t rsvd;
} __attribute__((packed, aligned(16)));

/**
 * @struct xhci_ring
 * @brief Producer/consumer state for a ring (command/transfer/event)
 */
struct xhci_ring {
	/** Virtual base of ring memory. */
	struct trb *base;
	/** Physical address of ring base. */
	uint64_t phys;
	/** Number of TRBs in the ring. */
	uint32_t num_trbs;
	/** Producer enqueue index (software). */
	uint32_t enqueue;
	/** Producer cycle state (PCS). */
	uint32_t cycle;
	/** Consumer cycle state (for event ring). */
	uint32_t ccs;
	/** Software consumer dequeue index (for event ring). */
	uint32_t dequeue;
};

/**
 * @struct ep_ctx
 * @brief Endpoint context as defined by the xHCI spec (32B form)
 */
struct ep_ctx {
	/** Endpoint attributes: MPS, Max Burst, type, error count. */
	uint32_t dword0;
	/** Interval, LSA, hid bits; EP-type lives in [5:3]. */
	uint32_t dword1;
	/** Transfer ring dequeue pointer (bit0 = DCS). */
	uint64_t deq;
	/** Average TRB length, Max ESIT payload, etc. */
	uint32_t dword4;
	/** Reserved. */
	uint32_t rsvd[3];
};

/**
 * @struct slot_ctx
 * @brief Slot context as defined by the xHCI spec (32B form)
 */
struct slot_ctx {
	/** Speed code and Context Entries count. */
	uint32_t dword0;
	/** Root port number (and route string for HS TT). */
	uint32_t dword1;
	/** TT hub/port or reserved (non-LS/FS). */
	uint32_t dword2;
	/** Assigned USB address and state. */
	uint32_t dword3;
	/** Reserved. */
	uint32_t rsvd[4];
};

/**
 * @struct input_ctx
 * @brief Input context used by Address/Configure/Evaluate commands
 *
 * Consists of two bitmaps (drop/add) followed by context structures
 * for slot and endpoints. This layout matches 32B contexts
 */
struct input_ctx {
	/** Drop Context flags bitmap. */
	uint32_t drop_flags;
	/** Add Context flags bitmap. */
	uint32_t add_flags;
	/** Reserved. */
	uint32_t rsvd[6];
	/** Slot context (target for updates). */
	struct slot_ctx slot;
	/** Default control endpoint (EP0) context. */
	struct ep_ctx ep0;
	/** EP1 OUT context */
	struct ep_ctx ep1_out;
	/** EP1 IN context */
	struct ep_ctx ep1_in;
	/** EP2 OUT context */
	struct ep_ctx ep2_out;
	/** EP2 IN context */
	struct ep_ctx ep2_in;
} __attribute__((packed, aligned(64)));

/**
 * @typedef xhci_int_in_cb
 * @brief Callback invoked on interrupt-IN completion
 *
 * @param ud  Device handle as seen by class drivers
 * @param pkt Pointer to received report bytes
 * @param len Number of valid bytes in @p pkt
 */
typedef void (*xhci_int_in_cb)(struct usb_dev *ud, const uint8_t *pkt, uint16_t len);

/**
 * @struct xhci_slot_state
 * @brief Per-slot runtime state to support multiple devices
 *
 * Mirrors the “legacy single device” fields, but one per Slot ID
 */
struct xhci_slot_state {
	/** xHCI Slot ID (1..max_slots). */
	int slot_id;

	/** Input context pointer (software). */
	struct input_ctx *ic;
	/** Physical address of input context. */
	uint64_t ic_phys;

	/** EP0 transfer ring (control transfers). */
	struct xhci_ring ep0_tr;

	/** Interrupt-IN transfer ring (e.g., HID). */
	struct xhci_ring int_in_tr;
	/** Interrupt-IN completion callback. */
	xhci_int_in_cb int_cb;
	/** Expected report size for INT-IN. */
	uint16_t int_pkt_len;
	/** DMA buffer for interrupt reports. */
	uint8_t *int_buf;
	/** Physical address of @ref int_buf. */
	uint64_t int_buf_phys;

	/** Discovered interrupt IN endpoint number (1..15). */
	uint8_t int_ep_num;
	/** wMaxPacketSize for interrupt IN endpoint. */
	uint16_t int_ep_mps;
	/** bInterval for interrupt IN endpoint. */
	uint8_t int_ep_interval;

	/** Slot allocation flag (1 = active). */
	bool in_use;
	/** Assigned USB address (if tracked). */
	uint8_t dev_addr;
	/** Root port number (1-based). */
	int port;
	/** Link speed code (per xHCI). */
	int speed;

	/** Control transfer bounce buffer (DMA-safe). */
	uint8_t *ctrl_dma;
	/** Physical address of @ref ctrl_dma. */
	uint64_t ctrl_dma_phys;
	/** Size of @ref ctrl_dma. */
	uint16_t ctrl_dma_sz;

	struct xhci_ring bulk_in_tr;
	struct xhci_ring bulk_out_tr;
	uint8_t bulk_in_num;
	uint8_t bulk_out_num;
	uint16_t bulk_in_mps;
	uint16_t bulk_out_mps;

};

/**
 * @struct xhci_hc
 * @brief Root object for an xHCI host controller instance
 */
struct xhci_hc {
	/** CAP (capability) MMIO base. */
	volatile uint8_t *cap;
	/** OP (operational) MMIO base. */
	volatile uint8_t *op;
	/** Doorbell MMIO base. */
	volatile uint8_t *db;
	/** Runtime MMIO base. */
	volatile uint8_t *rt;

	/** Capability length (offset to OP). */
	uint8_t cap_len;
	/** Maximum slots supported by the controller. */
	uint8_t max_slots;
	/** 1 if 64B contexts supported; we use 32B for v0. */
	uint8_t csz64;
	/** Number of root ports. */
	uint8_t port_count;

	/** Command ring state. */
	struct xhci_ring cmd;
	/** Event ring (single segment) state. */
	struct xhci_ring evt;

	/** Event Ring Segment Table pointer. */
	struct erst_entry *erst;
	/** Physical address of ERST. */
	uint64_t erst_phys;

	/** Device Context Base Address Array (DCBAA). */
	uint64_t *dcbaa;
	/** Physical address of DCBAA. */
	uint64_t dcbaa_phys;
	/** Scratchpad buffer array (index). */
	uint64_t *scratch_index;
	/** Physical address of scratchpad buffer array. */
	uint64_t scratch_index_phys;

	/** Legacy INTx line number. */
	uint8_t irq_line;
	/** Interrupt pin (A..D mapped to 1..4). */
	uint8_t irq_pin;

	/** Per-slot runtime state, index by Slot ID (1..max_slots). */
	struct xhci_slot_state slots[256];
};

/**
 * @brief Probe and initialise a single xHCI controller
 * @param bus  PCI bus number
 * @param dev  PCI device number
 * @param func PCI function number
 * @return true on success, false on failure
 */
bool xhci_probe_and_init(uint8_t bus, uint8_t dev, uint8_t func);

/**
 * @brief Submit a synchronous control transfer on EP0
 *
 * @param ud          Device handle
 * @param setup       Pointer to 8-byte setup packet
 * @param data        IN/OUT data buffer (or NULL if none)
 * @param len         Data length in bytes (0 for no data stage)
 * @param data_dir_in 1 for IN (device->host), 0 for OUT (host->device)
 * @return true on success, false on timeout/failure
 */
bool xhci_ctrl_xfer(const struct usb_dev *ud, const uint8_t *setup, void *data, uint16_t len, int data_dir_in);

/**
 * @brief Arm or re-arm an interrupt-IN endpoint for continuous reports
 *
 * @param ud       Device handle
 * @param pkt_len  Expected report size (e.g., 8 for HID boot keyboard)
 * @param cb       Callback invoked on each completion
 * @return true on success, false on failure
 */
bool xhci_arm_int_in(const struct usb_dev *ud, uint16_t pkt_len, xhci_int_in_cb cb);

/**
 * @brief Locate, map, and start the first xHCI controller; register poller
 */
void init_usb_xhci(void);

/**
 * @brief Issue a control transfer with no data stage (wLength = 0)
 *
 * @param ud     Device handle
 * @param bm     bmRequestType
 * @param bReq   bRequest
 * @param wValue wValue
 * @param wIndex wIndex
 * @return true on success, false on failure
 */
bool usb_ctrl_nodata(const struct usb_dev *ud, uint8_t bm, uint8_t bReq, uint16_t wValue, uint16_t wIndex);

/**
 * @brief Issue a control IN transfer (device -> host)
 *
 * @param ud     Device handle
 * @param bm     bmRequestType
 * @param bReq   bRequest
 * @param wValue wValue
 * @param wIndex wIndex
 * @param buf    Destination buffer (len bytes)
 * @param len    Number of bytes to read
 * @return true on success, false on failure
 */
bool usb_ctrl_get(const struct usb_dev *ud, uint8_t bm, uint8_t bReq, uint16_t wValue, uint16_t wIndex, void *buf, uint16_t len);

/**
 * @brief Issue a control OUT transfer (host -> device)
 *
 * @param ud     Device handle
 * @param bm     bmRequestType
 * @param bReq   bRequest
 * @param wValue wValue
 * @param wIndex wIndex
 * @param buf    Source buffer (len bytes)
 * @param len    Number of bytes to write
 * @return true on success, false on failure
 */
bool usb_ctrl_set(const struct usb_dev *ud, uint8_t bm, uint8_t bReq, uint16_t wValue, uint16_t wIndex, const void *buf, uint16_t len);

/**
 * @brief Match exact class/subclass/protocol triplet
 * @param ud_c        Device descriptor as seen by class drivers
 * @param dev_class   bInterfaceClass
 * @param dev_subclass bInterfaceSubClass
 * @param dev_proto   bInterfaceProtocol
 * @return true if all fields match, else false
 */
static inline bool is_class_and_protocol(const struct usb_dev *ud_c, uint8_t dev_class, uint8_t dev_subclass, uint8_t dev_proto) {
	return (ud_c->dev_class == dev_class && ud_c->dev_subclass == dev_subclass && ud_c->dev_proto == dev_proto);
}

/**
 * @brief Match class + subclass (any protocol)
 * @param ud_c        Device descriptor as seen by class drivers
 * @param dev_class   bInterfaceClass
 * @param dev_subclass bInterfaceSubClass
 * @return true if both match, else false
 */
static inline bool is_class_and_subclass(const struct usb_dev *ud_c, uint8_t dev_class, uint8_t dev_subclass) {
	return (ud_c->dev_class == dev_class && ud_c->dev_subclass == dev_subclass);
}

/* Open/configure a BULK endpoint (EP1 OUT or EP1 IN) for the given slot.
 * Creates a transfer ring if missing and issues CONFIGURE_ENDPOINT.
 * NOTE: For now we only support ep_num == 1 to match input_ctx layout.
 */
bool xhci_open_bulk_pipe(const struct usb_dev *ud, uint8_t ep_num, int dir_in, uint16_t mps);

/* Submit a single BULK transfer (IN or OUT) on the already-opened pipe.
 * Polled wait for a matching Transfer Event. Accepts SUCCESS and (for IN)
 * SHORT_PACKET. Buffer must be DMA-safe (kmalloc_aligned).
 */
bool xhci_bulk_xfer(const struct usb_dev *ud, int dir_in, void *buf, uint32_t len);
