/* usb_hid.c */
#include "usb_hid.h"

static void hid_keyboard_report_cb(struct usb_dev *ud, const uint8_t *pkt, uint16_t len) {
	if (len < 8u) {
		return;
	}
	/* pkt[2..7] are keycodes; translate via simple table, raise input events */
	/* Minimal: stash last key, push to your input layer. */
}

static void hid_on_device_added(const struct usb_dev *ud) {
	if (ud->dev_class != USB_CLASS_HID) {
		return;
	}
	/* Boot interface path only for v0 */
	if (ud->dev_subclass == USB_SUBCLASS_BOOT && ud->dev_proto == USB_PROTO_KEYBOARD) {
		/* SET_PROTOCOL(0)=Boot, SET_IDLE(0), then arm INT-IN 8 bytes */
		uint8_t setup[8];

		/* SET_PROTOCOL (Class, Interface) bm=0x21, bReq=0x0B, wValue=0, wIndex=interface, wLength=0
		   We have a single interface in v0; wIndex=0. */
		setup[0] = 0x21u; setup[1] = 0x0Bu;
		setup[2] = 0u;    setup[3] = 0u;     /* wValue */
		setup[4] = 0u;    setup[5] = 0u;     /* wIndex */
		setup[6] = 0u;    setup[7] = 0u;     /* wLength */
		(void) xhci_ctrl_xfer(ud, setup, 0, 0, 1);

		/* SET_IDLE (0) bm=0x21, bReq=0x0A */
		setup[0] = 0x21u; setup[1] = 0x0Au;
		setup[2] = 0u;    setup[3] = 0u;
		setup[4] = 0u;    setup[5] = 0u;
		setup[6] = 0u;    setup[7] = 0u;
		(void) xhci_ctrl_xfer(ud, setup, 0, 0, 1);

		/* Arm 8-byte boot report stream */
		(void) xhci_arm_int_in(ud, 8u, hid_keyboard_report_cb);
	}
}

static void hid_on_device_removed(const struct usb_dev *ud) {
	(void)ud;
	/* Tear-down if you keep per-device state */
}

static struct usb_class_ops hid_ops = {
	.name = "hid",
	.on_device_added = hid_on_device_added,
	.on_device_removed = hid_on_device_removed,
};

void usb_hid_init(void) {
	usb_core_register_class(USB_CLASS_HID, &hid_ops);
}
