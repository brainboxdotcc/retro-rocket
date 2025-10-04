#ifndef USB_CORE_H
#define USB_CORE_H

#include <kernel.h>
#include <stdint.h>
#include <stddef.h>

/* ---- USB basics ---- */

typedef enum {
	USB_EP_CONTROL  = 0,
	USB_EP_ISOCH    = 1,
	USB_EP_BULK     = 2,
	USB_EP_INTERRUPT= 3
} usb_ep_type_t;

/* Minimal endpoint model (xHCI uses “EPID” numbering: 1=EP0, 2/3=EP1 OUT/IN, …) */
typedef struct usb_endpoint {
	uint8_t  epid;      /* xHCI endpoint ID (1 = EP0) */
	uint16_t mps;       /* max packet size */
	uint8_t  type;      /* usb_ep_type_t */
	uint8_t  dir_in;    /* 1 = IN, 0 = OUT (for control this is 0) */
} usb_endpoint_t;

/* Device record published by HCDs (xhci, ehci, uhci…) */
typedef struct usb_dev {
	void    *hc;            /* host controller opaque pointer (xhci_hc*) */
	uint8_t  slot_id;       /* xHCI slot ID */
	uint8_t  address;       /* USB address (post-set-address) */

	uint16_t vid, pid;      /* vendor/product */
	uint8_t  dev_class;     /* bInterfaceClass (from selected interface) */
	uint8_t  dev_subclass;  /* bInterfaceSubClass */
	uint8_t  dev_proto;     /* bInterfaceProtocol */

	usb_endpoint_t ep0;     /* default control endpoint */
} usb_dev_t;

/* ------------------------------------------------------------------
 * Core API consumed by HCDs (xHCI) and class drivers (HID, MSC, …)
 * ------------------------------------------------------------------ */

/* Called by the HCD when a new device is successfully enumerated. */
void usb_core_device_added(const usb_dev_t *dev);

/* Lookup helpers used by class drivers. */
#define USB_CLASS_HID   0x03

/* --- USB class driver callbacks --- */

struct usb_dev; /* forward declaration only */

struct usb_class_ops {
	const char *name;
	void (*on_device_added)(const struct usb_dev *dev);
	void (*on_device_removed)(const struct usb_dev *dev);
};

int usb_core_register_class(uint8_t class_code, const struct usb_class_ops *ops);

/* Return pointer to first device with matching class/subclass/proto (0xFF = don't care). */
usb_dev_t *usb_core_find_by_class(uint8_t cls, uint8_t subcls, uint8_t proto);

/* Simple iterator over all devices (index 0..n-1). Returns NULL when out of range. */
usb_dev_t *usb_core_get_index(size_t index);

/* Count of currently registered devices. */
size_t usb_core_device_count(void);

/* Optional: textual dump for debugging. */
void usb_core_dump(void);

#endif /* USB_CORE_H */
