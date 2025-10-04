#include <kernel.h>
#include "usb_core.h"

/* ============================================================
 * Tiny USB core — class registry + device notifications
 * ============================================================ */

#define MAX_CLASS_SLOTS 16
#define MAX_DEVICES     8   /* small pool; host driver can grow this */

struct class_slot {
	uint8_t class_code;
	struct usb_class_ops *ops;
};

static struct class_slot g_classes[MAX_CLASS_SLOTS];
static struct usb_dev g_devices[MAX_DEVICES];
static size_t g_dev_count = 0;


/* ------------------------------------------------------------
 * Public core API
 * ------------------------------------------------------------ */
void usb_core_init(void) {
	memset(g_classes, 0, sizeof(g_classes));
	memset(g_devices, 0, sizeof(g_devices));
	g_dev_count = 0;
	dprintf("usb-core: initialised\n");
}

int usb_core_register_class(uint8_t class_code, const struct usb_class_ops *ops) {
	for (size_t i = 0; i < MAX_CLASS_SLOTS; i++) {
		if (g_classes[i].ops == NULL) {
			g_classes[i].class_code = class_code;
			g_classes[i].ops = (struct usb_class_ops *)ops;
			dprintf("usb-core: registered class 0x%02x (%s)\n",
				class_code, ops && ops->name ? ops->name : "anon");
			return 1;
		}
	}
	dprintf("usb-core: WARNING - class table full; 0x%02x not registered\n", class_code);
	return 0;
}

/* host driver calls this when it has a ready-to-use device */
void usb_core_device_added(const struct usb_dev *dev) {
	if (!dev) return;

	if (g_dev_count < MAX_DEVICES) {
		g_devices[g_dev_count++] = *dev;
	}

	for (size_t i = 0; i < MAX_CLASS_SLOTS; i++) {
		if (!g_classes[i].ops) continue;
		if (g_classes[i].class_code == dev->dev_class) {
			if (g_classes[i].ops->on_device_added) {
				g_classes[i].ops->on_device_added(dev);
			}
		}
	}
}

void usb_core_device_removed(const struct usb_dev *dev) {
	if (!dev) return;

	for (size_t i = 0; i < MAX_CLASS_SLOTS; i++) {
		if (!g_classes[i].ops) continue;
		if (g_classes[i].class_code == dev->dev_class) {
			if (g_classes[i].ops->on_device_removed) {
				g_classes[i].ops->on_device_removed(dev);
			}
		}
	}

	for (size_t i = 0; i < g_dev_count; i++) {
		if (g_devices[i].slot_id == dev->slot_id && g_devices[i].address == dev->address) {
			for (size_t j = i + 1; j < g_dev_count; j++) {
				g_devices[j - 1] = g_devices[j];
			}
			g_dev_count--;
			break;
		}
	}
}
