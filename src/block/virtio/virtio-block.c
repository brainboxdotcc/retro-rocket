#include <kernel.h>

/* ============================================================
 * Virtio Block (Virtio 1.0+, modern only) — Complete Driver
 * BAR4-centric: capability offsets resolved against BAR4 base
 * ============================================================ */

static int virtio_block_hw_enable(virtio_block_dev_t *v, pci_dev_t pdev);
static int virtio_block_probe_caps(virtio_block_dev_t *v, pci_dev_t pdev);
static int virtio_block_setup_queue0(virtio_block_dev_t *v);
static int virtio_block_rw(virtio_block_dev_t *v, bool write, uint64_t lba512, uint32_t count512, void *buf);
static void virtio_block_notify(virtio_block_dev_t *v, uint16_t qid);
static int storage_device_virtio_block_read(void *dev_ptr, uint64_t start, uint32_t bytes, unsigned char *buffer);
static int storage_device_virtio_block_write(void *dev_ptr, uint64_t start, uint32_t bytes, const unsigned char *buffer);

/* ------------------------------------------------------------
 * Enable PCI device and identity-map BAR4 (MMIO)
 * ------------------------------------------------------------ */
static int virtio_block_hw_enable(virtio_block_dev_t *v, pci_dev_t pdev) {
	uint32_t bar4 = pci_read(pdev, PCI_BAR4);
	uint32_t bar5 = pci_read(pdev, PCI_BAR5);

	if (!pci_bar_is_mem64(bar4)) {
		dprintf("virtio-block: BAR4 is not 64-bit MMIO\n");
		return 0;
	}

	uint64_t mmio_base = pci_mem_base64(bar4, bar5);
	uint64_t mmio_size = get_bar_size(pdev, 4);

	dprintf("virtio-block: MMIO base 0x0000%lx size %lu\n", mmio_base, mmio_size);

	pci_enable_memspace(pdev);
	pci_bus_master(pdev);

	if (!mmio_identity_map(mmio_base, mmio_size)) {
		dprintf("virtio-block: identity map failed\n");
		return 0;
	}

	v->bar_base = (volatile uint8_t *)(uintptr_t) mmio_base;
	v->bar_size = mmio_size;
	return 1;
}

/* --- raw PCI config readers (byte-accurate via CF8/CFC) --- */
static inline uint32_t pci_cfg_addr(pci_dev_t dev, uint32_t off) {
	return 0x80000000u
	       | ((uint32_t)dev.bus_num     << 16)
	       | ((uint32_t)dev.device_num  << 11)
	       | ((uint32_t)dev.function_num<<  8)
	       |  (off & 0xFCu);
}

static inline uint32_t pci_cfg_read32_raw(pci_dev_t dev, uint32_t off) {
	uint32_t addr = pci_cfg_addr(dev, off);
	outl(PCI_CONFIG_ADDRESS, addr);
	return inl(PCI_CONFIG_DATA);
}

static inline uint8_t pci_cfg_read8_raw(pci_dev_t dev, uint32_t off) {
	uint32_t v = pci_cfg_read32_raw(dev, off & ~3u);
	return (uint8_t)((v >> ((off & 3u) * 8u)) & 0xFFu);
}

/* --- complete, corrected capability discovery (modern virtio) --- */
static int virtio_block_probe_caps(virtio_block_dev_t *v, pci_dev_t pdev) {
	uint32_t sts = pci_read(pdev, PCI_STATUS);
	if ((sts & PCI_STATUS_CAPABILITIES_LIST) == 0) {
		dprintf("virtio-block: PCI capabilities not present\n");
		return 0;
	}

	/* 0x34 is an 8-bit byte offset; mask low 2 reserved bits */
	uint8_t cap_ptr = (uint8_t)(pci_cfg_read8_raw(pdev, PCI_CAPABILITY_POINTER) & 0xFCu);

	/* map each BAR once on demand */
	volatile uint8_t *bar_base_cache[6] = { 0 };

	while (cap_ptr) {
		/* vendor cap header is 16 bytes (4 dwords) starting at cap_ptr */
		uint32_t w0 = pci_cfg_read32_raw(pdev, cap_ptr + 0);
		uint32_t w1 = pci_cfg_read32_raw(pdev, cap_ptr + 4);
		uint32_t w2 = pci_cfg_read32_raw(pdev, cap_ptr + 8);
		uint32_t w3 = pci_cfg_read32_raw(pdev, cap_ptr + 12);

		virtio_pci_cap_hdr_t h;
		((uint32_t *)&h)[0] = w0;
		((uint32_t *)&h)[1] = w1;
		((uint32_t *)&h)[2] = w2;
		((uint32_t *)&h)[3] = w3;

		uint8_t next = h.cap_next;

		if (h.cap_vndr == 0x09) {
			volatile uint8_t *bar = bar_base_cache[h.bar];
			if (!bar) {
				uint32_t lo = pci_read(pdev, PCI_BAR0 + h.bar * 4);
				uint32_t hi = pci_read(pdev, PCI_BAR0 + h.bar * 4 + 4);
				if ((lo & 1u) != 0) {
					dprintf("virtio-block: BAR%u is I/O (need MMIO)\n", h.bar);
					return 0;
				}
				uint64_t base = pci_bar_is_mem64(lo) ? pci_mem_base64(lo, hi) : (uint64_t)(lo & ~0xFu);
				uint64_t size = get_bar_size(pdev, h.bar);
				if (!mmio_identity_map(base, size)) {
					dprintf("virtio-block: identity map failed for BAR%u\n", h.bar);
					return 0;
				}
				bar = (volatile uint8_t *)(uintptr_t)base;
				bar_base_cache[h.bar] = bar;
			}

			if (h.cfg_type == VIRTIO_PCI_CAP_COMMON_CFG) {
				v->common = (volatile virtio_pci_common_cfg_t *)(bar + h.offset);
				dprintf("virtio-block: COMMON @ BAR%u+0x%x\n", h.bar, h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_DEVICE_CFG) {
				v->device_cfg = (volatile virtio_block_config_t *)(bar + h.offset);
				dprintf("virtio-block: DEVICE @ BAR%u+0x%x\n", h.bar, h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_ISR_CFG) {
				v->isr = (volatile uint8_t *)(bar + h.offset);
				dprintf("virtio-block: ISR @ BAR%u+0x%x\n", h.bar, h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG) {
				uint32_t mul = pci_cfg_read32_raw(pdev, cap_ptr + 16);
				v->notify_base = (volatile uint8_t *)(bar + h.offset);
				v->notify_off_mul = mul;
				dprintf("virtio-block: NOTIFY @ BAR%u+0x%x (mul=%u)\n", h.bar, h.offset, v->notify_off_mul);
			}
		}

		cap_ptr = next & 0xFCu;
	}

	if (!v->common || !v->device_cfg || !v->notify_base) {
		dprintf("virtio-block: missing required caps (COMMON/DEVICE/NOTIFY)\n");
		return 0;
	}
	return 1;
}

/* ------------------------------------------------------------
 * Queue doorbell
 * ------------------------------------------------------------ */
static void virtio_block_notify(virtio_block_dev_t *v, uint16_t qid) {
	uint16_t notify_off = v->common->queue_notify_off;
	volatile uint16_t *doorbell = (volatile uint16_t *) (v->notify_base + (uint32_t) notify_off * v->notify_off_mul);
	*doorbell = qid;
}

/* ------------------------------------------------------------
 * Set up queue 0, allocate rings, DRIVER_OK
 * ------------------------------------------------------------ */
static int virtio_block_setup_queue0(virtio_block_dev_t *v) {
	volatile virtio_pci_common_cfg_t *c = v->common;

	/* Reset */
	c->device_status = 0;
	while (c->device_status != 0) { /* spin */ }

	/* ACK + DRIVER */
	c->device_status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;

	/* Conservative features: none */
	c->device_feature_select = 0;
	(void)c->device_feature;
	c->driver_feature_select = 0;
	c->driver_feature = 0;

	c->device_feature_select = 1;
	(void)c->device_feature;
	c->driver_feature_select = 1;
	c->driver_feature = 0;

	/* FEATURES_OK */
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
	v->q_size = qs;
	v->q_mask = (uint16_t) (qs - 1);

	/* Allocate rings (round to 4 KiB) */
	size_t desc_bytes = sizeof(virtq_desc_t) * qs;
	size_t avail_bytes = sizeof(virtq_avail_t) + sizeof(uint16_t) * qs;
	size_t used_bytes  = sizeof(virtq_used_t)  + sizeof(virtq_used_elem_t) * qs;

	size_t desc_sz  = (desc_bytes  + 4095u) & ~4095u;
	size_t avail_sz = (avail_bytes + 4095u) & ~4095u;
	size_t used_sz  = (used_bytes  + 4095u) & ~4095u;

	v->desc  = (virtq_desc_t *) kmalloc_aligned(desc_sz, 4096);
	v->avail = (virtq_avail_t *) kmalloc_aligned(avail_sz, 4096);
	v->used  = (virtq_used_t  *) kmalloc_aligned(used_sz,  4096);
	if (!v->desc || !v->avail || !v->used) {
		dprintf("virtio-block: ring alloc failed\n");
		return 0;
	}
	memset((void *) v->desc,  0, desc_sz);
	memset((void *) v->avail, 0, avail_sz);
	memset((void *) v->used,  0, used_sz);

	v->desc_phys  = (uint64_t) (uintptr_t) v->desc;
	v->avail_phys = (uint64_t) (uintptr_t) v->avail;
	v->used_phys  = (uint64_t) (uintptr_t) v->used;

	/* Program queue addresses */
	c->queue_desc   = v->desc_phys;
	c->queue_driver = v->avail_phys;
	c->queue_device = v->used_phys;

	/* Enable queue */
	c->queue_enable = 1;

	/* Scratch req buffers */
	v->hdr = (virtio_block_req_hdr_t *) kmalloc_aligned(4096, 4096);
	v->st  = (virtio_block_req_status_t *) kmalloc_aligned(4096, 4096);
	if (!v->hdr || !v->st) {
		dprintf("virtio-block: req buffers alloc failed\n");
		return 0;
	}

	/* Device config */
	v->capacity_512  = v->device_cfg ? v->device_cfg->capacity : 0;
	v->logical_block = (v->device_cfg && v->device_cfg->block_size != 0) ? v->device_cfg->block_size : 512u;

	/* DRIVER_OK */
	c->device_status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
			   VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK;

	dprintf("virtio-block: Q0 size=%u  capacity=%lu*512  block_size=%u\n", v->q_size, v->capacity_512, v->logical_block);
	return 1;
}

/* ------------------------------------------------------------
 * Single request (IN/OUT)
 * ------------------------------------------------------------ */
static int virtio_block_rw(virtio_block_dev_t *v, bool write, uint64_t lba512, uint32_t count512, void *buf) {
	if (count512 == 0) {
		return 1;
	}

	/* 3-descriptor chain: [hdr] -> [data] -> [status] */
	memset(v->hdr, 0, sizeof(*v->hdr));
	v->hdr->type   = write ? VIRTIO_BLOCK_T_OUT : VIRTIO_BLOCK_T_IN;
	v->hdr->sector = lba512;

	v->desc[0].addr  = (uint64_t) (uintptr_t) v->hdr;
	v->desc[0].len   = sizeof(*v->hdr);
	v->desc[0].flags = VIRTQ_DESC_F_NEXT;
	v->desc[0].next  = 1;

	v->desc[1].addr  = (uint64_t) (uintptr_t) buf;
	v->desc[1].len   = count512 * 512u;
	v->desc[1].flags = (write ? 0 : VIRTQ_DESC_F_WRITE) | VIRTQ_DESC_F_NEXT;
	v->desc[1].next  = 2;

	v->desc[2].addr  = (uint64_t) (uintptr_t) v->st;
	v->desc[2].len   = sizeof(*v->st);
	v->desc[2].flags = VIRTQ_DESC_F_WRITE;

	uint16_t aidx = v->avail->idx;
	v->avail->ring[aidx & v->q_mask] = 0; /* head of chain */
	__asm__ volatile("sfence" ::: "memory");
	v->avail->idx = (uint16_t) (aidx + 1);

	virtio_block_notify(v, 0);

	/* Poll Used */
	while (v->used->idx == v->used_last) {
		__asm__ volatile("pause");
	}
	v->used_last++;

	if (v->st->status != 0) {
		dprintf("virtio-block: I/O status=%u\n", v->st->status);
		return 0;
	}
	return 1;
}

/* ------------------------------------------------------------
 * Storage layer callbacks (bytes API over 512B sectors)
 * ------------------------------------------------------------ */
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
		uint32_t this512 = (sectors > 256) ? 256 : sectors;
		if (!virtio_block_rw(v, false, start, this512, p)) {
			fs_set_error(FS_ERR_IO);
			return 0;
		}
		start   += this512;
		p       += (size_t) this512 * 512u;
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
		start   += this512;
		p       += (size_t) this512 * 512u;
		sectors -= this512;
	}
	return 1;
}

/* ------------------------------------------------------------
 * Entry point (modern-only)
 * ------------------------------------------------------------ */
void init_virtio_block(void) {
	pci_dev_t dev = pci_get_device(VIRTIO_VENDOR_ID, VIRTIO_DEVICE_BLOCK_MODERN, -1);
	if (!dev.bits) {
		dprintf("virtio-block: no modern virtio-blk device\n");
		return;
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
	sd->cache   = NULL;

	sd->blockread  = storage_device_virtio_block_read;
	sd->blockwrite = storage_device_virtio_block_write;
	sd->blockclear = NULL;

	sd->block_size = 512;
	sd->size       = vb->capacity_512;

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
