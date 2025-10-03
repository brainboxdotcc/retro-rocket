#include <kernel.h>

static int virtio_block_hw_enable(virtio_block_dev_t *v, pci_dev_t pdev);
static int virtio_block_probe_caps(virtio_block_dev_t *v, pci_dev_t pdev);
static int virtio_block_setup_queue0(virtio_block_dev_t *v);
static int virtio_block_rw(virtio_block_dev_t *v, bool write, uint64_t lba512, uint32_t count512, void *buf);
static void virtio_block_notify(virtio_block_dev_t *v, uint16_t qid);

static int virtio_block_hw_enable(virtio_block_dev_t *v, pci_dev_t pdev) {
	uint32_t bar4 = pci_read(pdev, PCI_BAR4);
	uint32_t bar5 = pci_read(pdev, PCI_BAR5);

	if (!pci_bar_is_mem64(bar4)) {
		dprintf("virtio-block: BAR4 is not 64-bit MMIO\n");
		return 0;
	}

	uint64_t mmio_base = pci_mem_base64(bar4, bar5);
	uint64_t mmio_size = get_bar_size(pdev, 4);

	dprintf("virtio-block: MMIO base 0x%016lx size %lu\n", mmio_base, mmio_size);

	pci_enable_memspace(pdev);
	pci_bus_master(pdev);

	if (!mmio_identity_map(mmio_base, mmio_size)) {
		dprintf("virtio-block: identity map failed\n");
		return 0;
	}

	v->bar_base = (volatile uint8_t *) (uintptr_t) mmio_base;
	v->bar_size = mmio_size;
	return 1;
}

static int virtio_block_probe_caps(virtio_block_dev_t *v, pci_dev_t pdev) {
	/* Check if the function even has capabilities */
	uint32_t sts_cmd = pci_read(pdev, 1);
	if (((sts_cmd >> 16) & (1u << 4)) == 0) {
		dprintf("virtio-block: PCI capabilities not present\n");
		return 0;
	}

	/* Read pointer to first capability */
	uint32_t cap_ptr_dw = pci_read(pdev, 13);
	uint8_t cap_ptr = (uint8_t) (cap_ptr_dw & 0xFF) & 0xFC; /* DWORD aligned */

	volatile uint8_t *bar4 = v->bar_base;

	while (cap_ptr) {
		uint32_t raw0 = pci_read(pdev, cap_ptr / 4);
		virtio_pci_cap_hdr_t h;
		/* We need the whole header; read two dwords to get the first 8 bytes; then two more for offset/length */
		/* Safer: read via pci_read for the four dwords */
		((uint32_t *) &h)[0] = raw0;
		((uint32_t *) &h)[1] = pci_read(pdev, cap_ptr / 4 + 1);
		((uint32_t *) &h)[2] = pci_read(pdev, cap_ptr / 4 + 2);
		((uint32_t *) &h)[3] = pci_read(pdev, cap_ptr / 4 + 3);

		uint8_t next = h.cap_next;

		if (h.cap_vndr == 0x09) {
			if (h.cfg_type == VIRTIO_PCI_CAP_COMMON_CFG) {
				if (h.bar != 4) {
					dprintf("virtio-block: COMMON on BAR%u unsupported\n", h.bar);
					return 0;
				}
				v->common = (volatile virtio_pci_common_cfg_t *) (bar4 + h.offset);
				dprintf("virtio-block: COMMON @ +0x%x\n", h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_DEVICE_CFG) {
				if (h.bar != 4) {
					dprintf("virtio-block: DEVICE on BAR%u unsupported\n", h.bar);
					return 0;
				}
				v->device_cfg = (volatile virtio_block_config_t *) (bar4 + h.offset);
				dprintf("virtio-block: DEVICE @ +0x%x\n", h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_ISR_CFG) {
				if (h.bar != 4) {
					dprintf("virtio-block: ISR on BAR%u unsupported\n", h.bar);
					return 0;
				}
				v->isr = (volatile uint8_t *) (bar4 + h.offset);
				dprintf("virtio-block: ISR @ +0x%x\n", h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG) {
				/* This cap is larger; re-read as notify-cap to get the multiplier */
				virtio_pci_notify_cap_t n;
				((uint32_t *) &n)[0] = ((uint32_t *) &h)[0];
				((uint32_t *) &n)[1] = ((uint32_t *) &h)[1];
				((uint32_t *) &n)[2] = ((uint32_t *) &h)[2];
				((uint32_t *) &n)[3] = ((uint32_t *) &h)[3];
				((uint32_t *) &n)[4] = pci_read(pdev, cap_ptr / 4 + 4);

				if (n.cap.bar != 4) {
					dprintf("virtio-block: NOTIFY on BAR%u unsupported\n", n.cap.bar);
					return 0;
				}
				v->notify_base = (volatile uint8_t *) (bar4 + n.cap.offset);
				v->notify_off_mul = n.notify_off_multiplier;
				dprintf("virtio-block: NOTIFY @ +0x%x (mul=%u)\n", n.cap.offset, v->notify_off_mul);
			}
		}

		cap_ptr = next;
	}

	if (!v->common || !v->device_cfg || !v->notify_base) {
		dprintf("virtio-block: missing required caps (COMMON/DEVICE/NOTIFY)\n");
		return 0;
	}
	return 1;
}

static void virtio_block_notify(virtio_block_dev_t *v, uint16_t qid) {
	uint16_t notify_off = v->common->queue_notify_off;
	volatile uint16_t *doorbell = (volatile uint16_t *) (v->notify_base + (uint32_t) notify_off * v->notify_off_mul);
	*doorbell = qid; /* value is queue index or any write (virtio spec allows either); QEMU accepts write */
}

static int virtio_block_setup_queue0(virtio_block_dev_t *v) {
	volatile virtio_pci_common_cfg_t *c = v->common;

	/* Reset */
	c->device_status = 0;
	while (c->device_status != 0) { /* spin */ }

	/* ACK + DRIVER */
	c->device_status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;

	/* Feature negotiation (we accept nothing fancy; plain block) */
	c->device_feature_select = 0;
	uint32_t devfeat0 = c->device_feature;
	(void) devfeat0; /* ignore for now */
	c->driver_feature_select = 0;
	c->driver_feature = 0;

	c->device_feature_select = 1;
	uint32_t devfeat1 = c->device_feature;
	(void) devfeat1;
	c->driver_feature_select = 1;
	c->driver_feature = 0;

	c->device_status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK;
	if ((c->device_status & VIRTIO_STATUS_FEATURES_OK) == 0) {
		dprintf("virtio-block: FEATURES_OK not latched\n");
		c->device_status = VIRTIO_STATUS_FAILED;
		return 0;
	}

	/* Queue 0 discovery */
	c->queue_select = 0;
	uint16_t qs = c->queue_size;
	if (qs == 0) {
		dprintf("virtio-block: queue0 size = 0\n");
		return 0;
	}
	/* Clamp to sane minimum; power-of-two typical */
	v->q_size = qs;
	v->q_mask = (uint16_t) (qs - 1);

	/* Allocate rings: desc[qs], avail(4 + 2*qs), used(4 + 8*qs).
	   Round each to 4K for simplicity. */
	size_t desc_bytes = sizeof(virtq_desc_t) * qs;
	size_t avail_bytes = sizeof(virtq_avail_t) + sizeof(uint16_t) * qs; /* flags+idx+ring */
	size_t used_bytes = sizeof(virtq_used_t) + sizeof(virtq_used_elem_t) * qs;

	size_t desc_sz = (desc_bytes + 4095) & ~4095u;
	size_t avail_sz = (avail_bytes + 4095) & ~4095u;
	size_t used_sz = (used_bytes + 4095) & ~4095u;

	v->desc = (virtq_desc_t *) kmalloc_aligned(desc_sz, 4096);
	v->avail = (virtq_avail_t *) kmalloc_aligned(avail_sz, 4096);
	v->used = (virtq_used_t *) kmalloc_aligned(used_sz, 4096);
	if (!v->desc || !v->avail || !v->used) {
		dprintf("virtio-block: ring alloc failed\n");
		return 0;
	}
	memset((void *) v->desc, 0, desc_sz);
	memset((void *) v->avail, 0, avail_sz);
	memset((void *) v->used, 0, used_sz);

	v->desc_phys = (uint64_t) (uintptr_t) v->desc;
	v->avail_phys = (uint64_t) (uintptr_t) v->avail;
	v->used_phys = (uint64_t) (uintptr_t) v->used;

	/* Program queue addresses */
	c->queue_desc = v->desc_phys;
	c->queue_driver = v->avail_phys;
	c->queue_device = v->used_phys;

	/* Enable queue */
	c->queue_enable = 1;

	/* Scratch request header/footer (aligned 4K is overkill but easy) */
	v->hdr = (virtio_block_req_hdr_t *) kmalloc_aligned(4096, 4096);
	v->st = (virtio_block_req_status_t *) kmalloc_aligned(4096, 4096);
	if (!v->hdr || !v->st) {
		dprintf("virtio-block: req buffers alloc failed\n");
		return 0;
	}

	/* Free list implicit: we use 3 descriptors (hdr, data, status) as a fixed chain starting at 0 */
	v->free_head = 0;
	v->used_last = 0;

	/* Read block config */
	v->capacity_512 = v->device_cfg->capacity;
	v->logical_block = (v->device_cfg->block_size == 0) ? 512u : v->device_cfg->block_size;

	/* Driver ready */
	c->device_status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK;

	dprintf("virtio-block: Q0 size=%u  capacity=%lu*512  block_size=%u\n", v->q_size, v->capacity_512, v->logical_block);
	return 1;
}

static int virtio_block_rw(virtio_block_dev_t *v, bool write, uint64_t lba512, uint32_t count512, void *buf) {
	/* We operate in 512-byte units as exposed by virtio-block sector */
	if (count512 == 0) {
		return 1;
	}

	/* Build 3-descriptor chain: [hdr] -> [data] -> [status] */
	uint16_t i_hdr = 0;
	uint16_t i_dat = 1;
	uint16_t i_st = 2;

	/* Header */
	memset(v->hdr, 0, sizeof(*v->hdr));
	v->hdr->type = write ? VIRTIO_BLOCK_T_OUT : VIRTIO_BLOCK_T_IN;
	v->hdr->sector = lba512;

	v->desc[i_hdr].addr = (uint64_t) (uintptr_t) v->hdr;
	v->desc[i_hdr].len = sizeof(*v->hdr);
	v->desc[i_hdr].flags = VIRTQ_DESC_F_NEXT;
	v->desc[i_hdr].next = i_dat;

	/* Data */
	v->desc[i_dat].addr = (uint64_t) (uintptr_t) buf;
	v->desc[i_dat].len = count512 * 512u;
	v->desc[i_dat].flags = (write ? 0 : VIRTQ_DESC_F_WRITE) | VIRTQ_DESC_F_NEXT;
	v->desc[i_dat].next = i_st;

	/* Status (device writes 0 for success) */
	v->desc[i_st].addr = (uint64_t) (uintptr_t) v->st;
	v->desc[i_st].len = sizeof(*v->st);
	v->desc[i_st].flags = VIRTQ_DESC_F_WRITE;
	v->desc[i_st].next = 0;

	/* Avail */
	uint16_t aidx = v->avail->idx;
	v->avail->ring[aidx & v->q_mask] = i_hdr;
	__asm__ volatile("sfence":: : "memory");
	v->avail->idx = (uint16_t) (aidx + 1);

	/* Notify */
	virtio_block_notify(v, 0);

	/* Poll Used for completion */
	for (;;) {
		uint16_t uidx = v->used->idx;
		if (uidx != v->used_last) {
			uint16_t slot = v->used_last & v->q_mask;
			virtq_used_elem_t e = v->used->ring[slot];
			v->used_last = (uint16_t) (v->used_last + 1);
			(void) e;
			break;
		}
		__asm__ volatile("pause");
	}

	/* Check status byte */
	if (v->st->status != 0) {
		dprintf("virtio-block: I/O status=%u\n", v->st->status);
		return 0;
	}
	return 1;
}

static int storage_device_virtio_block_read(void *dev_ptr, uint64_t start, uint32_t bytes, unsigned char *buffer) {
	storage_device_t *sd = (storage_device_t *) dev_ptr;
	if (!sd || !buffer || bytes == 0) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return 0;
	}
	virtio_block_dev_t *v = (virtio_block_dev_t *) sd->opaque1;
	if (!v) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return 0;
	}

	uint32_t sectors = (bytes + 511u) / 512u;
	unsigned char *p = buffer;
	while (sectors) {
		/* Bound the transfer to avoid huge waits; 128KiB = 256 sectors feels fine */
		uint32_t this512 = (sectors > 256) ? 256 : sectors;
		if (!virtio_block_rw(v, false, start, this512, p)) {
			fs_set_error(FS_ERR_IO);
			return 0;
		}
		start += this512;
		p += (size_t) this512 * 512u;
		sectors -= this512;
	}
	return 1;
}

static int storage_device_virtio_block_write(void *dev_ptr, uint64_t start, uint32_t bytes, const unsigned char *buffer) {
	storage_device_t *sd = (storage_device_t *) dev_ptr;
	if (!sd || !buffer) {
		fs_set_error(FS_ERR_INVALID_ARG);
		return 0;
	}
	virtio_block_dev_t *v = (virtio_block_dev_t *) sd->opaque1;
	if (!v) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return 0;
	}

	uint32_t sectors = (bytes + 511u) / 512u;
	const unsigned char *p = buffer;
	while (sectors) {
		uint32_t this512 = (sectors > 256) ? 256 : sectors;
		if (!virtio_block_rw(v, true, start, this512, (void *) p)) {
			fs_set_error(FS_ERR_IO);
			return 0;
		}
		start += this512;
		p += (size_t) this512 * 512u;
		sectors -= this512;
	}
	return 1;
}

void init_virtio_block(void) {
	dprintf("Bringing up virtio-block...\n");

	/* Find a virtio-block device: we accept modern (0x1042) or legacy (0x1001) */
	pci_dev_t dev = pci_get_device(VIRTIO_VENDOR_ID, VIRTIO_DEVICE_BLOCK_MODERN, -1);
	if (!dev.bits) {
		dev = pci_get_device(VIRTIO_VENDOR_ID, VIRTIO_DEVICE_BLOCK_LEGACY, -1);
		if (!dev.bits) {
			dprintf("virtio-block: no device found\n");
			return;
		}
	}

	virtio_block_dev_t *vb = (virtio_block_dev_t *) kmalloc(sizeof(virtio_block_dev_t));
	if (!vb) {
		return;
	}
	memset(vb, 0, sizeof(*vb));

	if (!virtio_block_hw_enable(vb, dev)) {
		kfree(vb);
		return;
	}
	if (!virtio_block_probe_caps(vb, dev)) {
		kfree(vb);
		return;
	}
	if (!virtio_block_setup_queue0(vb)) {
		kfree(vb);
		return;
	}

	storage_device_t *sd = (storage_device_t *) kmalloc(sizeof(storage_device_t));
	if (!sd) {
		kfree(vb);
		return;
	}
	memset(sd, 0, sizeof(*sd));

	char name[16];
	if (!make_unique_device_name("hd", name, sizeof(name))) {
		kfree(sd);
		kfree(vb);
		return;
	}

	sd->opaque1 = vb;
	sd->opaque2 = NULL;
	sd->opaque3 = NULL;
	sd->cache = NULL;

	sd->blockread = storage_device_virtio_block_read;
	sd->blockwrite = storage_device_virtio_block_write;
	sd->blockclear = NULL;

	sd->block_size = 512;
	sd->size = vb->capacity_512;

	strlcpy(sd->name, name, sizeof(sd->name));

	/* Human label */
	char size_str[24] = {0};
	humanise_capacity(size_str, sizeof(size_str), vb->capacity_512 * 512ull);
	snprintf(sd->ui.label, sizeof(sd->ui.label), "Virtio Block - %s", size_str);
	sd->ui.is_optical = false;

	register_storage_device(sd);
	storage_enable_cache(sd);

	kprintf("Virtio storage: %s\n", sd->ui.label);
}
