/* usb_hid.c */
#include "usb_hid.h"

static void hid_keyboard_report_cb(struct usb_dev *ud, const uint8_t *pkt, uint16_t len) {
	if (len < 8u) {
		return;
	}
	dprintf("keyboard report cb\n");
	/* pkt[2..7] are keycodes; translate via simple table, raise input events */
	/* Minimal: stash last key, push to your input layer. */
}

/* Call this right after you arm EP1-IN (before “waiting for interrupts”). */
static void hid_try_get_report_snapshot(struct usb_dev *ud) {
	uint8_t __attribute__((aligned(64))) setup[8] = {
		0xA1, 0x01,        /* bmReqType=Class|Interface|IN, bRequest=GET_REPORT */
		0x01, 0x00,        /* wValue = (ReportType=Input=1)<<8 | ReportID=0 */
		0x00, 0x00,        /* wIndex = interface 0 */
		0x08, 0x00         /* wLength = 8 */
	};
	uint8_t __attribute__((aligned(64))) buf[8] = {0};
	int ok = xhci_ctrl_xfer(ud, setup, buf, 8, 1);
	dprintf("hid: GET_REPORT(Input,id=0) %s, data=%02x %02x %02x %02x %02x %02x %02x %02x\n",
		ok ? "OK" : "FAIL",
		buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
}

static void hid_toggle_numlock(struct usb_dev *ud, int on) {
	uint8_t __attribute__((aligned(64))) setup[8] = {
		0x21, 0x09,        /* bmReqType=Class|Interface|OUT, bRequest=SET_REPORT */
		0x02, 0x00,        /* wValue = (ReportType=Output=2)<<8 | ReportID=0 */
		0x00, 0x00,        /* wIndex = interface 0 */
		0x01, 0x00         /* wLength = 1 */
	};
	uint8_t __attribute__((aligned(64))) led = (on ? 0x01 : 0x00); /* bit0 = NumLock */
	int ok = xhci_ctrl_xfer(ud, setup, &led, 1, 0);
	dprintf("hid: SET_REPORT(Output LED NumLock=%d) %s\n", on, ok ? "OK" : "FAIL");
}

static void hid_on_device_added(const struct usb_dev *ud_c)
{
	dprintf("hid_on_device_added ud_c=%p\n", ud_c);
	if (!ud_c) return;

	/* Only handle HID Boot Keyboard */
	if (!(ud_c->dev_class == USB_CLASS_HID &&
	      ud_c->dev_subclass == USB_SUBCLASS_BOOT &&
	      ud_c->dev_proto == USB_PROTO_KEYBOARD)) {
		dprintf("hid: ignore dev class=%02x sub=%02x proto=%02x\n",
			ud_c->dev_class, ud_c->dev_subclass, ud_c->dev_proto);
		return;
	}

	/* xhci_ctrl_xfer expects non-const usb_dev* */
	struct usb_dev *ud = (struct usb_dev *)ud_c;
	uint8_t __attribute__((aligned(64))) setup[8];
	uint16_t iface = 0;

	dprintf("hid: attach keyboard VID:PID=%04x:%04x slot=%u\n",
		ud->vid, ud->pid, ud->slot_id);

	/* 1) SET_CONFIGURATION(1) */
	setup[0] = 0x00; setup[1] = 0x09; /* bm=Std Dev OUT, SET_CONFIGURATION */
	setup[2] = 0x01; setup[3] = 0x00; /* wValue = 1 */
	setup[4] = 0x00; setup[5] = 0x00; /* wIndex = 0 (device) */
	setup[6] = 0x00; setup[7] = 0x00; /* wLength = 0 */
	if (!xhci_ctrl_xfer(ud, setup, NULL, 0, 0)) {
		dprintf("hid: SET_CONFIGURATION(1) failed\n");
		return;
	}
	dprintf("hid: set configuration ok\n");

	/* 2) SET_PROTOCOL(BOOT) on interface 0
	      bmRequestType=0x21 (Class OUT, Interface), bRequest=0x0B, wValue=0 (BOOT) */
	setup[0] = 0x21; setup[1] = 0x0B;
	setup[2] = 0x00; setup[3] = 0x00;                /* wValue = 0 (BOOT) */
	setup[4] = (uint8_t)(iface & 0xFF);              /* wIndex = interface */
	setup[5] = (uint8_t)(iface >> 8);
	setup[6] = 0x00; setup[7] = 0x00;                /* wLength = 0 */
	if (!xhci_ctrl_xfer(ud, setup, NULL, 0, 0)) {
		dprintf("hid: SET_PROTOCOL(BOOT) failed\n");
		return;
	}
	dprintf("hid: set protocol(boot) ok\n");

	/* 3) SET_IDLE(0) on interface 0
	      bmRequestType=0x21 (Class OUT, Interface), bRequest=0x0A, wValue=0 (duration=0, reportId=0) */
	setup[0] = 0x21; setup[1] = 0x0A;
	setup[2] = 0x00; setup[3] = 0x00;                /* wValue = 0 */
	setup[4] = (uint8_t)(iface & 0xFF);
	setup[5] = (uint8_t)(iface >> 8);
	setup[6] = 0x00; setup[7] = 0x00;
	if (!xhci_ctrl_xfer(ud, setup, NULL, 0, 0)) {
		dprintf("hid: SET_IDLE(0) failed\n");
		return;
	}
	dprintf("hid: set idle(0) ok\n");

	/* 4) Arm EP1 IN for 8-byte boot reports */
	if (!xhci_arm_int_in(ud, 8u, hid_keyboard_report_cb)) {
		dprintf("hid: arm EP1 IN failed\n");
		return;
	}
	dprintf("hid: keyboard armed (EP1 IN), waiting for interrupts\n");

	hid_try_get_report_snapshot(ud);
	hid_toggle_numlock(ud, 1);
}

static void hid_on_device_removed(const struct usb_dev *ud) {
}

static struct usb_class_ops hid_ops = {
	.name = "hid",
	.on_device_added = hid_on_device_added,
	.on_device_removed = hid_on_device_removed,
};

void usb_hid_init(void) {
	usb_core_register_class(USB_CLASS_HID, &hid_ops);
}
