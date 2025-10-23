#include <kernel.h>

/* ============================================================
 * Virtio Block (Virtio 1.0+, modern only) - Complete Driver
 * BAR4-centric: capability offsets resolved against BAR4 base
 * ============================================================ */

static int virtio_block_hw_enable(virtio_block_dev_t *v, pci_dev_t pdev);

static int virtio_block_probe_caps(virtio_block_dev_t *v, pci_dev_t device);

static int virtio_block_setup_queue0(virtio_block_dev_t *v);

static int virtio_block_rw(virtio_block_dev_t *v, bool write, uint64_t lba512, uint32_t count512, void *buf);

static void virtio_block_notify(virtio_block_dev_t *v, uint16_t qid);

static int storage_device_virtio_block_read(void *dev_ptr, uint64_t start, uint32_t bytes, unsigned char *buffer);

static int storage_device_virtio_block_write(void *dev_ptr, uint64_t start, uint32_t bytes, const unsigned char *buffer);

/* single, page-aligned bounce buffer used when caller buffer isn't page-aligned */
static uint8_t *virtio_blk_bounce = NULL;
static size_t virtio_blk_bounce_sz = 0;


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

	v->bar_base = (volatile uint8_t *) (uintptr_t) mmio_base;
	v->bar_size = mmio_size;
	return 1;
}

/* --- raw PCI config readers (byte-accurate via CF8/CFC) --- */
static inline uint32_t pci_cfg_addr(pci_dev_t dev, uint32_t off) {
	return 0x80000000u
	       | ((uint32_t) dev.bus_num << 16)
	       | ((uint32_t) dev.device_num << 11)
	       | ((uint32_t) dev.function_num << 8)
	       | (off & 0xFCu);
}

static inline uint32_t pci_cfg_read32_raw(pci_dev_t dev, uint32_t off) {
	uint32_t addr = pci_cfg_addr(dev, off);
	outl(PCI_CONFIG_ADDRESS, addr);
	return inl(PCI_CONFIG_DATA);
}

static inline uint8_t pci_cfg_read8_raw(pci_dev_t dev, uint32_t offset) {
	uint32_t v = pci_cfg_read32_raw(dev, offset & ~3u);
	return (uint8_t) ((v >> ((offset & 3u) * 8u)) & 0xFFu);
}

/* --- complete, corrected capability discovery (modern virtio) --- */
static int virtio_block_probe_caps(virtio_block_dev_t *v, pci_dev_t device) {
	uint32_t sts = pci_read(device, PCI_STATUS);
	if ((sts & PCI_STATUS_CAPABILITIES_LIST) == 0) {
		dprintf("virtio-block: PCI capabilities not present\n");
		return 0;
	}

	/* 0x34 is an 8-bit byte offset; mask low 2 reserved bits */
	uint8_t cap_ptr = (uint8_t) (pci_cfg_read8_raw(device, PCI_CAPABILITY_POINTER) & 0xFCu);

	/* map each BAR once on demand */
	volatile uint8_t *bar_base_cache[6] = {0};

	while (cap_ptr) {
		/* vendor cap header is 16 bytes (4 dwords) starting at cap_ptr */
		uint32_t w0 = pci_cfg_read32_raw(device, cap_ptr + 0);
		uint32_t w1 = pci_cfg_read32_raw(device, cap_ptr + 4);
		uint32_t w2 = pci_cfg_read32_raw(device, cap_ptr + 8);
		uint32_t w3 = pci_cfg_read32_raw(device, cap_ptr + 12);

		virtio_pci_cap_hdr_t h;
		((uint32_t *) &h)[0] = w0;
		((uint32_t *) &h)[1] = w1;
		((uint32_t *) &h)[2] = w2;
		((uint32_t *) &h)[3] = w3;

		uint8_t next = h.cap_next;

		if (h.cap_vndr == 0x09) {
			volatile uint8_t *bar = bar_base_cache[h.bar];
			if (!bar) {
				uint32_t lo = pci_read(device, PCI_BAR0 + h.bar * 4);
				uint32_t hi = pci_read(device, PCI_BAR0 + h.bar * 4 + 4);
				if ((lo & 1u) != 0) {
					dprintf("virtio-block: BAR%u is I/O (need MMIO)\n", h.bar);
					return 0;
				}
				uint64_t base = pci_bar_is_mem64(lo) ? pci_mem_base64(lo, hi) : (uint64_t) (lo & ~0xFu);
				uint64_t size = get_bar_size(device, h.bar);
				mmio_identity_map(base, size);
				bar = (volatile uint8_t *) (uintptr_t) base;
				bar_base_cache[h.bar] = bar;
			}

			if (h.cfg_type == VIRTIO_PCI_CAP_COMMON_CFG) {
				v->common = (volatile virtio_pci_common_cfg_t *) (bar + h.offset);
				dprintf("virtio-block: COMMON @ BAR%u+0x%x\n", h.bar, h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_DEVICE_CFG) {
				v->device_cfg = (volatile virtio_block_config_t *) (bar + h.offset);
				dprintf("virtio-block: DEVICE @ BAR%u+0x%x\n", h.bar, h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_ISR_CFG) {
				v->isr = (volatile uint8_t *) (bar + h.offset);
				dprintf("virtio-block: ISR @ BAR%u+0x%x\n", h.bar, h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG) {
				uint32_t mul = pci_cfg_read32_raw(device, cap_ptr + 16);
				v->notify_base = (volatile uint8_t *) (bar + h.offset);
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

static void virtio_block_notify(virtio_block_dev_t *v, uint16_t queue_id) {
	v->common->queue_select = queue_id; /* ensure queue_notify_off reflects correct queue */
	uint16_t notify_off = v->common->queue_notify_off;
	volatile uint16_t *doorbell = (volatile uint16_t *) (v->notify_base + (uint32_t) notify_off * v->notify_off_mul);
	*doorbell = queue_id; /* value is queue index or any write (virtio spec allows either); QEMU accepts write */
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

	/* Allocate a single 4K-aligned bounce buffer sized for our max transfer (256 * 512 = 128KiB) */
	if (!virtio_blk_bounce) {
		virtio_blk_bounce_sz = 256u * 512u;
		virtio_blk_bounce = (uint8_t *) kmalloc_aligned(virtio_blk_bounce_sz, 4096);
		if (!virtio_blk_bounce) {
			dprintf("virtio-block: bounce alloc failed (%u bytes)\n", (unsigned) virtio_blk_bounce_sz);
			return 0;
		}
		dprintf("virtio-block: bounce buffer ready (%u bytes @ 4K aligned)\n", (unsigned) virtio_blk_bounce_sz);
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

	/* Ensure DMA-safe buffer: use the static 4K-aligned bounce if caller buffer isn't page-aligned */
	size_t total_bytes = count512 * 512u;
	void *dma_buf = buf;
	bool used_bounce = false;

	if (((uintptr_t) buf & 0xFFFu) != 0) {
		if (!virtio_blk_bounce || virtio_blk_bounce_sz < total_bytes) {
			dprintf("virtio-block: bounce not available or too small (need=%u have=%u)\n",
				(unsigned) total_bytes, (unsigned) virtio_blk_bounce_sz);
			return 0;
		}
		if (write) {
			memcpy(virtio_blk_bounce, buf, total_bytes);
		}
		dma_buf = virtio_blk_bounce;
		used_bounce = true;
		dprintf("virtio-block: using bounce for %s (bytes=%u)\n", write ? "write" : "read", (unsigned) total_bytes);
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
	v->desc[i_dat].addr = (uint64_t) (uintptr_t) dma_buf;
	v->desc[i_dat].len = total_bytes;
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
		__builtin_ia32_pause();
	}

	if (v->st->status != 0) {
		dprintf("virtio-block: I/O status=%u\n", v->st->status);
		return 0;
	}

	/* Copy back from bounce on reads */
	if (used_bounce && !write) {
		memcpy(buf, virtio_blk_bounce, total_bytes);
	}

	return 1;
}

static int storage_device_virtio_block_read(void *dev_ptr, uint64_t start, uint32_t bytes, unsigned char *buffer) {
	storage_device_t *sd = (storage_device_t *) dev_ptr;
	if (!sd || !buffer || bytes == 0) {
		fs_set_error(FS_ERR_INVALID_ARG);
		dprintf("virtio-block: read invalid args (sd=%p buffer=%p bytes=%u)\n", sd, buffer, bytes);
		return 0;
	}

	virtio_block_dev_t *v = sd->opaque1;
	if (!v) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		dprintf("virtio-block: read failed (no device bound)\n");
		return 0;
	}

	if (v->capacity_512 == 0) {
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		dprintf("virtio-block: read failed (device capacity is zero)\n");
		return 0;
	}

	uint32_t sectors = (bytes + 511) / 512;
	if (sectors == 0) {
		dprintf("virtio-block: read zero sectors (bytes=%u) - nothing to do\n", bytes);
		return 1;
	}
	if (start > v->capacity_512 || sectors > v->capacity_512 - start) {
		fs_set_error(FS_ERR_INVALID_ARG);
		dprintf("virtio-block: read out of range (lba=%lu count=%u cap=%lu)\n",
			start, sectors, v->capacity_512);
		return 0;
	}

	unsigned char *p = buffer;
	uint64_t begin_lba = start;
	uint32_t total = sectors;

	while (sectors) {
		uint32_t this512 = (sectors > 256) ? 256 : sectors;

		uint32_t pre_csum = 0;
		size_t probe = (this512 * 512 < 16) ? (this512 * 512) : 16;
		for (size_t i = 0; i < probe; i++) {
			pre_csum = (pre_csum << 5) - pre_csum + p[i];
		}

		if (!virtio_block_rw(v, false, start, this512, p)) {
			fs_set_error(FS_ERR_IO);
			dprintf("virtio-block: read I/O error (lba=%lu count=%u chunk=%u/%u start=%lu)\n",
				start, this512, this512, total, begin_lba);
			return 0;
		}

		uint32_t post_csum = 0;
		for (size_t i = 0; i < probe; i++) {
			post_csum = (post_csum << 5) - post_csum + p[i];
		}

		if (post_csum == pre_csum) {
			dprintf("virtio-block: read suspicious (buffer unchanged) at lba=%lu count=%u probe16=0x%08x\n",
				start, this512, post_csum);
		} else {
			dprintf("virtio-block: read ok (lba=%lu count=%u) data[0..1]=%02x %02x probe16=0x%08x\n",
				start, this512, p[0], p[1], post_csum);
		}

		start += this512;
		p += this512 * 512;
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

	/* Basic range/size checking so we can surface errors early */
	if (v->capacity_512 == 0) {
		dprintf("virtio-block: write with zero capacity device\n");
		fs_set_error(FS_ERR_NO_SUCH_DEVICE);
		return 0;
	}
	if (bytes == 0) {
		return 1;
	}

	uint32_t sectors = (bytes + 511u) / 512u;
	/* Range check: ensure we don't pass end of device */
	if (start > v->capacity_512 || sectors > v->capacity_512 - start) {
		dprintf("virtio-block: write out of range (lba=%lu, count=%u, cap=%lu)\n",
			(unsigned long) start, (unsigned) sectors, (unsigned long) v->capacity_512);
		fs_set_error(FS_ERR_INVALID_ARG);
		return 0;
	}

	const unsigned char *p = buffer;
	while (sectors) {
		uint32_t this512 = (sectors > 256) ? 256 : sectors;
		if (!virtio_block_rw(v, true, start, this512, (void *) p)) {
			/* virtio_block_rw already dprintf’ed status/timeout; map to FS error */
			fs_set_error(FS_ERR_IO);
			return 0;
		}
		start += this512;
		p += (size_t) this512 * 512u;
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
