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
static struct usb_dev *g_devices = NULL;
static bool *g_dev_bound = NULL;
static size_t g_dev_count = 0;


/* ------------------------------------------------------------
 * Public core API
 * ------------------------------------------------------------ */
void usb_core_init(void) {
	/* allocate fixed-size slabs on the heap */
	g_classes = kmalloc_aligned(MAX_CLASS_SLOTS * sizeof(struct class_slot), 64);
	g_devices = kmalloc_aligned(MAX_DEVICES * sizeof(struct usb_dev), 64);
	g_dev_bound = kmalloc_aligned(MAX_DEVICES * sizeof(bool), 64);
	if (!g_classes || !g_devices || !g_dev_bound) {
		dprintf("usb-core: ERROR - allocation failed (classes=%p devices=%p)\n", g_classes, g_devices);
		kfree_null(&g_classes);
		kfree_null(&g_devices);
		kfree_null(&g_dev_bound);
		g_dev_count = 0;
		return;
	}
	memset(g_classes, 0, MAX_CLASS_SLOTS * sizeof(struct class_slot));
	memset(g_devices, 0, MAX_DEVICES * sizeof(struct usb_dev));
	memset(g_dev_bound, 0, MAX_DEVICES * sizeof(bool));
	g_dev_count = 0;

	dprintf("usb-core: initialised\n");
}

void usb_core_rescan(void) {
	if (!g_devices || !g_classes || !g_dev_bound) {
		return;
	}

	for (size_t i = 0; i < g_dev_count; i++) {
		if (g_dev_bound[i]) {
			continue;
		}
		uint8_t cls = g_devices[i].dev_class;

		for (size_t k = 0; k < MAX_CLASS_SLOTS; k++) {
			if (!g_classes[k].ops) {
				continue;
			}
			if (g_classes[k].class_code == cls) {
				if (g_classes[k].ops->on_device_added) {
					g_classes[k].ops->on_device_added(&g_devices[i]);
					g_dev_bound[i] = true;
				}
				break; /* single driver per class */
			}
		}
	}
}

bool usb_core_register_class(uint8_t class_code, const struct usb_class_ops *ops) {
	for (size_t i = 0; i < MAX_CLASS_SLOTS; i++) {
		if (g_classes[i].ops == NULL) {
			g_classes[i].class_code = class_code;
			g_classes[i].ops = (struct usb_class_ops *) ops;
			dprintf("usb-core: registered class 0x%02x (%s)\n", class_code, ops && ops->name ? ops->name : "anon");
			usb_core_rescan();
			return true;
		}
	}
	dprintf("usb-core: WARNING - class table full; 0x%02x not registered\n", class_code);
	return false;
}

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

	/* Copy into our table; mark as not yet bound. */
	g_devices[g_dev_count] = *dev;
	if (g_dev_bound) {
		g_dev_bound[g_dev_count] = false;
	}

	/* Try to find a class driver now; if found, deliver exactly once and mark bound. */
	for (size_t i = 0; i < MAX_CLASS_SLOTS; i++) {
		if (!g_classes[i].ops) {
			continue;
		}
		if (g_classes[i].class_code == dev->dev_class) {
			if (g_classes[i].ops->on_device_added) {
				g_classes[i].ops->on_device_added(&g_devices[g_dev_count]);
				if (g_dev_bound) {
					g_dev_bound[g_dev_count] = true;
				}
			}
			break; /* single driver per class */
		}
	}

	g_dev_count++;
}

void usb_core_device_removed(const struct usb_dev *dev) {
	if (!dev) {
		return;
	}

	dprintf("USB CORE: Device removed: slot=%d class=%x subclass=%x\n", dev->slot_id, dev->dev_class, dev->dev_subclass);

	/* Notify the driver that was bound to this class, if any. */
	for (size_t i = 0; i < MAX_CLASS_SLOTS; i++) {
		if (!g_classes[i].ops) {
			continue;
		}
		if (g_classes[i].class_code == dev->dev_class) {
			if (g_classes[i].ops->on_device_removed) {
				g_classes[i].ops->on_device_removed(dev);
			}
			break; /* single driver per class */
		}
	}

	/* Remove from tables and keep arrays compact */
	for (size_t i = 0; i < g_dev_count; i++) {
		if (g_devices[i].slot_id == dev->slot_id && g_devices[i].address == dev->address) {
			for (size_t j = i + 1; j < g_dev_count; j++) {
				g_devices[j - 1] = g_devices[j];
				if (g_dev_bound) {
					g_dev_bound[j - 1] = g_dev_bound[j];
				}
			}
			if (g_dev_bound) {
				g_dev_bound[g_dev_count - 1] = false;
			}
			g_dev_count--;
			break;
		}
	}
}

/* OPTIONAL: call if you have a teardown path */
void usb_core_shutdown(void) {
	kfree_null(&g_classes);
	kfree_null(&g_devices);
	g_dev_count = 0;
	dprintf("usb-core: shutdown\n");
}
