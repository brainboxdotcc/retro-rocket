#pragma once
#include <kernel.h>
#include "usb_core.h"
#include "usb_xhci.h"

#define USB_SUBCLASS_BOOT   0x01
#define USB_PROTO_KEYBOARD  0x01

void usb_hid_init(void);
