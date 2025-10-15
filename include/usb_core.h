#pragma once

#include <kernel.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @enum usb_ep_type_t
 * @brief USB endpoint transfer type codes.
 */
typedef enum {
	/** Control endpoint (EP0, setup/data/status). */
	USB_EP_CONTROL  = 0,
	/** Isochronous endpoint (time-sensitive streaming). */
	USB_EP_ISOCH    = 1,
	/** Bulk endpoint (large, burst transfers). */
	USB_EP_BULK     = 2,
	/** Interrupt endpoint (periodic, low-latency). */
	USB_EP_INTERRUPT= 3
} usb_ep_type_t;

/**
 * @struct usb_endpoint
 * @brief Minimal endpoint model for xHCI and class drivers.
 *
 * xHCI uses “EPID” numbering:
 *  - 1 = EP0 (control)
 *  - 2 = EP1 OUT
 *  - 3 = EP1 IN
 *  - ...
 */
typedef struct usb_endpoint {
	/** xHCI endpoint ID. */
	uint8_t  epid;
	/** Maximum packet size. */
	uint16_t mps;
	/** Endpoint type (see @ref usb_ep_type_t). */
	uint8_t  type;
	/** Direction: 1 = IN (device->host), 0 = OUT (host->device). */
	uint8_t  dir_in;
} usb_endpoint_t;

/**
 * @struct usb_dev
 * @brief Published record for a USB device enumerated by the HCD.
 *
 * Carries class/interface identity and endpoint 0 information.
 */
typedef struct usb_dev {
	/** Host controller opaque pointer (xhci_hc*). */
	void    *hc;
	/** Assigned xHCI Slot ID (1..max_slots). */
	uint8_t  slot_id;
	/** Assigned USB device address (after SET_ADDRESS). */
	uint8_t  address;

	/** Vendor ID from device descriptor. */
	uint16_t vid;
	/** Product ID from device descriptor. */
	uint16_t pid;
	/** Interface class code (bInterfaceClass). */
	uint8_t  dev_class;
	/** Interface subclass code (bInterfaceSubClass). */
	uint8_t  dev_subclass;
	/** Interface protocol code (bInterfaceProtocol). */
	uint8_t  dev_proto;
	/** Interface number (bInterfaceNumber). */
	uint8_t  iface_num;

	/** Default control endpoint (EP0). */
	usb_endpoint_t ep0;
} usb_dev_t;

/* ============================================================
 * Core USB framework API (shared by HCDs and class drivers)
 * ============================================================ */

/**
 * @brief Notify the core that a new device has been enumerated.
 *
 * Called by host controller drivers once device enumeration succeeds.
 * Dispatches to registered class driver handlers.
 *
 * @param dev Pointer to published device record.
 */
void usb_core_device_added(const usb_dev_t *dev);

/** @brief Standard class code for HID devices. */
#define USB_CLASS_HID   0x03

/**
 * @typedef device_notify_change_t
 * @brief Callback type for class drivers on add/remove.
 *
 * @param dev Pointer to device being added or removed.
 */
typedef void (*device_notify_change_t)(const struct usb_dev *dev);

/**
 * @struct usb_class_ops
 * @brief Registration block for a USB class driver.
 */
struct usb_class_ops {
	/** Name string for diagnostics/logging. */
	const char *name;
	/** Called when a matching device is added. */
	device_notify_change_t on_device_added;
	/** Called when a matching device is removed. */
	device_notify_change_t on_device_removed;
};

/**
 * @brief Register a new class driver.
 *
 * @param class_code Class code to match (e.g. @ref USB_CLASS_HID).
 * @param ops        Operations block with callbacks.
 * @return true if registered, false if failed (e.g. duplicate).
 */
bool usb_core_register_class(uint8_t class_code, const struct usb_class_ops *ops);

/**
 * @brief Find the first device matching class/subclass/proto.
 *
 * 0xFF can be used as a wildcard (“don’t care”).
 *
 * @param cls    Class code.
 * @param subcls Subclass code.
 * @param proto  Protocol code.
 * @return Pointer to matching device, or NULL if none.
 */
usb_dev_t *usb_core_find_by_class(uint8_t cls, uint8_t subcls, uint8_t proto);

/**
 * @brief Get device by index.
 *
 * @param index Zero-based index of enumerated device.
 * @return Pointer to device or NULL if out of range.
 */
usb_dev_t *usb_core_get_index(size_t index);

/**
 * @brief Count the number of currently registered devices.
 * @return Device count.
 */
size_t usb_core_device_count(void);

/**
 * @brief Dump current USB devices and class drivers to log.
 */
void usb_core_dump(void);

/**
 * @brief Initialise USB core subsystem (class driver registry etc).
 */
void usb_core_init(void);

/**
 * @brief Re-scan all enumerated devices against the registered class drivers.
 *
 * This function replays class-driver binding for any devices that were
 * previously enumerated but not yet delivered to a matching driver.
 * It is typically called after a new class driver is registered
 * (e.g. usb_hid_init) so that existing devices can be bound
 * without requiring re-enumeration at the hardware level.
 *
 * Devices already marked as bound are skipped, so drivers will not
 * receive duplicate add-notifications.
 */
void usb_core_rescan(void);
