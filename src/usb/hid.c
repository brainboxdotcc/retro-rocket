/* usb_hid.c */
#include "usb_hid.h"

static uint8_t last_report[8];

/* sink for raw scancodes (make = code, break = code|0x80) */
void keyboard_process_scancode_input(uint8_t sc);

/* last 8-byte HID boot report snapshot */
static uint8_t last_report[8];

static inline void emit_make(uint8_t code)  {
	if (code) {
		keyboard_process_scancode_input(code);
	}
}
static inline void emit_break(uint8_t code) {
	if (code) {
		keyboard_process_scancode_input((uint8_t)(code | 0x80u));
	}
}

/* USB HID Boot Keyboard usage -> PS/2 Set-1 MAKE code (no break bit).
 * 0 means "unmapped/ignore".
 */
static const uint8_t hid_to_set1[0xE8] = {
	/* Letters a..z (0x04..0x1D) */
	[0x04]=0x1E, /* A */ [0x05]=0x30, /* B */ [0x06]=0x2E, /* C */ [0x07]=0x20, /* D */
	[0x08]=0x12, /* E */ [0x09]=0x21, /* F */ [0x0A]=0x22, /* G */ [0x0B]=0x23, /* H */
	[0x0C]=0x17, /* I */ [0x0D]=0x24, /* J */ [0x0E]=0x25, /* K */ [0x0F]=0x26, /* L */
	[0x10]=0x32, /* M */ [0x11]=0x31, /* N */ [0x12]=0x18, /* O */ [0x13]=0x19, /* P */
	[0x14]=0x10, /* Q */ [0x15]=0x13, /* R */ [0x16]=0x1F, /* S */ [0x17]=0x14, /* T */
	[0x18]=0x16, /* U */ [0x19]=0x2F, /* V */ [0x1A]=0x11, /* W */ [0x1B]=0x2D, /* X */
	[0x1C]=0x15, /* Y */ [0x1D]=0x2C, /* Z */

	/* Number row 1..0 (0x1E..0x27) */
	[0x1E]=0x02,[0x1F]=0x03,[0x20]=0x04,[0x21]=0x05,[0x22]=0x06,
	[0x23]=0x07,[0x24]=0x08,[0x25]=0x09,[0x26]=0x0A,[0x27]=0x0B,

	/* Control / punctuation (0x28..0x38) */
	[0x28]=0x1C, /* Enter */
	[0x29]=0x01, /* Escape */
	[0x2A]=0x0E, /* Backspace */
	[0x2B]=0x0F, /* Tab */
	[0x2C]=0x39, /* Space */
	[0x2D]=0x0C, /* - */
	[0x2E]=0x0D, /* = */
	[0x2F]=0x1A, /* [ */
	[0x30]=0x1B, /* ] */
	[0x31]=0x2B, /* \ (US) */
	/* 0x32 = Non-US #/~ (ISO “102nd key”): Set-1 = 0x56 on ISO keyboards */
	[0x32]=0x56,
	[0x33]=0x27, /* ; */
	[0x34]=0x28, /* ' */
	[0x35]=0x29, /* ` */
	[0x36]=0x33, /* , */
	[0x37]=0x34, /* . */
	[0x38]=0x35, /* / */

	/* Locks & F-keys */
	[0x39]=0x3A, /* Caps Lock */
	[0x3A]=0x3B, /* F1  */ [0x3B]=0x3C, /* F2  */ [0x3C]=0x3D, /* F3  */
	[0x3D]=0x3E, /* F4  */ [0x3E]=0x3F, /* F5  */ [0x3F]=0x40, /* F6  */
	[0x40]=0x41, /* F7  */ [0x41]=0x42, /* F8  */ [0x42]=0x43, /* F9  */
	[0x43]=0x44, /* F10 */ [0x44]=0x57, /* F11 */ [0x45]=0x58, /* F12 */

	/* Print/Scroll/Pause */
	/* [0x46] PrintScreen -> E0 2A E0 37 (complex), leave 0 for now */
	[0x47]=0x46, /* Scroll Lock */
	/* [0x48] Pause -> complex, leave unmapped here */

	/* Insert/Home/PgUp/Delete/End/PgDn & arrows (all extended: prefix 0xE0) */
	[0x49]=0x52, /* Insert  -> E0 52 */
	[0x4A]=0x47, /* Home    -> E0 47 */
	[0x4B]=0x49, /* PageUp  -> E0 49 */
	[0x4C]=0x53, /* Delete  -> E0 53 */
	[0x4D]=0x4F, /* End     -> E0 4F */
	[0x4E]=0x51, /* PageDn  -> E0 51 */
	[0x4F]=0x4D, /* Right   -> E0 4D */
	[0x50]=0x4B, /* Left    -> E0 4B */
	[0x51]=0x50, /* Down    -> E0 50 */
	[0x52]=0x48, /* Up      -> E0 48 */

	/* Keypad cluster */
	[0x53]=0x45, /* Num Lock */
	[0x54]=0x35, /* KP /    -> E0 35 (extended) */
	[0x55]=0x37, /* KP *    */
	[0x56]=0x4A, /* KP -    */
	[0x57]=0x4E, /* KP +    */
	[0x58]=0x1C, /* KP Enter -> E0 1C (extended) */
	[0x59]=0x4F, /* KP 1 */ [0x5A]=0x50, /* KP 2 */ [0x5B]=0x51, /* KP 3 */
	[0x5C]=0x4B, /* KP 4 */ [0x5D]=0x4C, /* KP 5 */ [0x5E]=0x4D, /* KP 6 */
	[0x5F]=0x47, /* KP 7 */ [0x60]=0x48, /* KP 8 */ [0x61]=0x49, /* KP 9 */
	[0x62]=0x52, /* KP 0 */ [0x63]=0x53, /* KP . */

	/* Application/Menu (extended) & Non-US \| on ISO boards */
	[0x65]=0x5D, /* Application -> E0 5D */

	/* Modifiers (L* are normal, R* are extended in Set-1) */
	[0xE0]=0x1D, /* LCtrl */
	[0xE1]=0x2A, /* LShift */
	[0xE2]=0x38, /* LAlt */
	[0xE3]=0x5B, /* LGUI (Win) -> E0 5B */
	[0xE4]=0x1D, /* RCtrl      -> E0 1D */
	[0xE5]=0x36, /* RShift */
	[0xE6]=0x38, /* RAlt (AltGr) -> E0 38 */
	[0xE7]=0x5C, /* RGUI          -> E0 5C */
};

/* Which HID usages should be emitted with an 0xE0 prefix in Set-1? */
static inline int hid_usage_needs_e0(uint8_t u)
{
	switch (u) {
		/* arrows + edit cluster */
		case 0x49: case 0x4A: case 0x4B: case 0x4C:
		case 0x4D: case 0x4E: case 0x4F: case 0x50:
		case 0x51: case 0x52:
			/* keypad / and Enter */
		case 0x54: case 0x58:
			/* right-side modifiers and GUI keys */
		case 0xE4: case 0xE6: case 0xE3: case 0xE7:
			return 1;
		default:
			return 0;
	}
}

static inline uint8_t hid_usage_to_set1(uint8_t u)
{
	return (u < sizeof(hid_to_set1)) ? hid_to_set1[u] : 0;
}

static int usage_present(const uint8_t *rep, uint8_t usage)
{
	for (int i = 2; i < 8; ++i) if (rep[i] == usage) return 1;
	return 0;
}

static uint64_t up = 0;

static void process_mod_changes(uint8_t prev_mod, uint8_t cur_mod)
{
	uint8_t changed = (uint8_t)(prev_mod ^ cur_mod);
	while (changed) {
		uint8_t bit = (uint8_t)(changed & (uint8_t)(-((int8_t)changed)));
		int idx = __builtin_ctz(changed);             /* 0..7 */
		uint8_t usage = (uint8_t)(0xE0u + idx);
		uint8_t make  = hid_usage_to_set1(usage);
		if (cur_mod & bit) {
			emit_make(make);
		} else {
			emit_break(make);
		}
		changed = (uint8_t)(changed & (changed - 1));
	}
}

static uint64_t ints;

static void hid_keyboard_report_cb(struct usb_dev *ud, const uint8_t *pkt, uint16_t len)
{
	if (len < 8u) return;

	//dprintf("enter %lu\n", ints++);

	/* disable HID’s built-in key repeat */
	if (memcmp(pkt, last_report, 8) == 0) {
		//dprintf("leave rep\n");
		return;
	}

	/* 1) modifiers (byte 0) */
	process_mod_changes(last_report[0], pkt[0]);

	/* 2) key array (bytes 2..7): releases first */
	for (int i = 2; i < 8; ++i) {
		uint8_t u = last_report[i];
		if (u && !usage_present(pkt, u)) {
			uint8_t mk = hid_usage_to_set1(u);
			emit_break(mk);
		}
	}
	/* then presses */
	for (int i = 2; i < 8; ++i) {
		uint8_t u = pkt[i];
		if (u && !usage_present(last_report, u)) {
			uint8_t mk = hid_usage_to_set1(u);
			emit_make(mk);
		}
	}

	memcpy(last_report, pkt, 8);

	//dprintf("leave\n");
}

static void hid_on_device_added(const struct usb_dev *ud_c)
{
	dprintf("hid_on_device_added ud_c=%p\n", ud_c);
	if (!ud_c) return;

	/* Only handle HID Boot Keyboard */
	if (!(ud_c->dev_class == USB_CLASS_HID && ud_c->dev_subclass == USB_SUBCLASS_BOOT && ud_c->dev_proto == USB_PROTO_KEYBOARD)) {
		dprintf("hid: ignore dev class=%02x sub=%02x proto=%02x\n", ud_c->dev_class, ud_c->dev_subclass, ud_c->dev_proto);
		return;
	}

	/* xhci_ctrl_xfer expects non-const usb_dev* */
	struct usb_dev *ud = (struct usb_dev *)ud_c;
	uint8_t __attribute__((aligned(64))) setup[8];
	uint16_t iface = 0;

	dprintf("hid: attach keyboard VID:PID=%04x:%04x slot=%u\n", ud->vid, ud->pid, ud->slot_id);

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
