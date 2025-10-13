#include <kernel.h>
#include "usb_core.h"

/* ============================================================
 * Tiny USB core - class registry + device notifications
 * ============================================================ */

#define MAX_CLASS_SLOTS 16
#define MAX_DEVICES     8   /* small pool; host driver can grow this */

struct class_slot {
	uint8_t class_code;
	struct usb_class_ops *ops;
};

/* CHANGED: statics -> heap pointers */
static struct class_slot *g_classes = NULL;
static struct usb_dev    *g_devices = NULL;
static size_t g_dev_count = 0;


/* ------------------------------------------------------------
 * Public core API
 * ------------------------------------------------------------ */
void usb_core_init(void) {
	/* allocate fixed-size slabs on the heap */
	g_classes = kmalloc_aligned(MAX_CLASS_SLOTS * sizeof(struct class_slot), 64);
	g_devices = kmalloc_aligned(MAX_DEVICES * sizeof(struct usb_dev), 64);
	if (!g_classes || !g_devices) {
		dprintf("usb-core: ERROR - allocation failed (classes=%p devices=%p)\n", g_classes, g_devices);
		if (g_classes) {
			kfree(g_classes);
			g_classes = NULL;
		}
		if (g_devices) {
			kfree(g_devices);
			g_devices = NULL;
		}
		g_dev_count = 0;
		return;
	}
	memset(g_classes, 0, MAX_CLASS_SLOTS * sizeof(struct class_slot));
	memset(g_devices, 0, MAX_DEVICES * sizeof(struct usb_dev));
	g_dev_count = 0;

	dprintf("usb-core: initialised\n");
}

int usb_core_register_class(uint8_t class_code, const struct usb_class_ops *ops) {
	for (size_t i = 0; i < MAX_CLASS_SLOTS; i++) {
		if (g_classes[i].ops == NULL) {
			g_classes[i].class_code = class_code;
			g_classes[i].ops = (struct usb_class_ops *)ops;
			dprintf("usb-core: registered class 0x%02x (%s)\n", class_code, ops && ops->name ? ops->name : "anon");
			return 1;
		}
	}
	dprintf("usb-core: WARNING - class table full; 0x%02x not registered\n", class_code);
	return 0;
}

/* host driver calls this when it has a ready-to-use device */
void usb_core_device_added(const struct usb_dev *dev) {
	if (!dev) {
		return;
	}

	dprintf("USB CORE: Device added: slot=%d class=%x subclass=%x dev=%p\n",
		dev->slot_id, dev->dev_class, dev->dev_subclass, dev);

	if (g_dev_count >= MAX_DEVICES) {
		dprintf("usb-core: WARNING - device table full; drop slot=%d addr=%d\n",
			dev->slot_id, dev->address);
		return;
	}

	g_devices[g_dev_count] = *dev;

	for (size_t i = 0; i < MAX_CLASS_SLOTS; i++) {
		if (!g_classes[i].ops) {
			continue;
		}
		if (g_classes[i].class_code == dev->dev_class) {
			if (g_classes[i].ops->on_device_added) {
				g_classes[i].ops->on_device_added(&g_devices[g_dev_count]);
			}
		}
	}

	g_dev_count++;
}


void usb_core_device_removed(const struct usb_dev *dev) {
	if (!dev) {
		return;
	}

	dprintf("USB CORE: Device removed: slot=%d class=%x subclass=%x\n", dev->slot_id, dev->dev_class, dev->dev_subclass);

	for (size_t i = 0; i < MAX_CLASS_SLOTS; i++) {
		if (!g_classes[i].ops) {
			continue;
		}
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

/* OPTIONAL: call if you have a teardown path */
void usb_core_shutdown(void) {
	if (g_classes) {
		kfree(g_classes);
		g_classes = NULL;
	}
	if (g_devices) {
		kfree(g_devices);
		g_devices = NULL;
	}
	g_dev_count = 0;
	dprintf("usb-core: shutdown\n");
}
