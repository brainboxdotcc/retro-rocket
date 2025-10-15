#pragma once

#include <kernel.h>
#include "usb_core.h"
#include "usb_xhci.h"

/** @brief HID Boot Interface Subclass code. */
#define USB_SUBCLASS_BOOT   0x01
/** @brief HID Boot Protocol code for keyboards. */
#define USB_PROTO_KEYBOARD  0x01

/** @brief USB HID 'boot' protocol */
#define HID_PROTO_BOOT 0x0000

/** @brief USB HID 'report' protocol */
#define HID_PROTO_REPORT 0x0001

/**
 * @brief Initialise the HID class driver.
 *
 * Registers HID class ops with the USB core.
 * Currently implements Boot Keyboard support only.
 */
void usb_hid_init(void);
