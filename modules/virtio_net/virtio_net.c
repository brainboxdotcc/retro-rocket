#include <kernel.h>
#include "virtio_net.h"

static bool virtio_net_hw_enable(pci_dev_t pdev) {
	uint32_t bar4 = pci_read(pdev, PCI_BAR4);
	uint32_t bar5 = pci_read(pdev, PCI_BAR5);

	if (!pci_bar_is_mem64(bar4)) {
		dprintf("virtio-net: BAR4 is not 64-bit MMIO\n");
		return false;
	}

	uint64_t mmio_base = pci_mem_base64(bar4, bar5);
	uint64_t mmio_size = get_bar_size(pdev, 4);

	dprintf("virtio-net: MMIO base 0x%lx size %lu\n", mmio_base, mmio_size);

	pci_enable_memspace(pdev);
	pci_bus_master(pdev);
	mmio_identity_map(mmio_base, mmio_size);

	vnet.bar_base[4] = (volatile uint8_t *) (uintptr_t) mmio_base;
	vnet.bar_size[4] = mmio_size;

	return true;
}

static bool virtio_net_probe_caps(pci_dev_t dev) {
	volatile uint8_t *bar_base_cache[6];
	memset(bar_base_cache, 0, sizeof(bar_base_cache));
	uint32_t sts = pci_read(dev, PCI_STATUS);

	if ((sts & PCI_STATUS_CAPABILITIES_LIST) == 0) {
		dprintf("virtio-net: PCI capabilities not present\n");
		return false;
	}

	uint8_t cap_ptr = pci_read8(dev, PCI_CAPABILITY_POINTER) & 0xFC;
	while (cap_ptr) {
		uint32_t w0 = pci_read32(dev, cap_ptr + 0);
		uint32_t w1 = pci_read32(dev, cap_ptr + 4);
		uint32_t w2 = pci_read32(dev, cap_ptr + 8);
		uint32_t w3 = pci_read32(dev, cap_ptr + 12);

		virtio_pci_cap_hdr_t h;
		((uint32_t *) &h)[0] = w0;
		((uint32_t *) &h)[1] = w1;
		((uint32_t *) &h)[2] = w2;
		((uint32_t *) &h)[3] = w3;

		uint8_t next = h.cap_next;

		if (h.cap_vndr == 0x09) {
			volatile uint8_t *bar = bar_base_cache[h.bar];

			if (!bar) {
				uint32_t lo = pci_read32(dev, PCI_BAR0 + h.bar * 4);
				uint32_t hi = pci_read32(dev, PCI_BAR0 + h.bar * 4 + 4);

				if ((lo & 1) != 0) {
					dprintf("virtio-net: BAR%u is I/O (need MMIO)\n", h.bar);
					return false;
				}

				uint64_t base;
				if (pci_bar_is_mem64(lo)) {
					base = pci_mem_base64(lo, hi);
				} else {
					base = (uint64_t) (lo & ~0xF);
				}

				size_t size = get_bar_size(dev, h.bar);

				mmio_identity_map(base, size);

				bar = (volatile uint8_t *) (uintptr_t) base;
				bar_base_cache[h.bar] = bar;
				vnet.bar_base[h.bar] = bar;
				vnet.bar_size[h.bar] = size;
			}

			if (h.cfg_type == VIRTIO_PCI_CAP_COMMON_CFG) {
				vnet.common = (volatile virtio_pci_common_cfg_t *) (bar + h.offset);
				dprintf("virtio-net: COMMON @ BAR%u+0x%x\n", h.bar, h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_DEVICE_CFG) {
				vnet.devcfg = (volatile virtio_net_config_t *) (bar + h.offset);
				dprintf("virtio-net: DEVICE @ BAR%u+0x%x\n", h.bar, h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_ISR_CFG) {
				vnet.isr = (volatile virtio_pci_isr_t *) (bar + h.offset);
				dprintf("virtio-net: ISR @ BAR%u+0x%x\n", h.bar, h.offset);
			} else if (h.cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG) {
				uint32_t mul = pci_read32(dev, cap_ptr + 16);
				vnet.notify_base = bar + h.offset;
				vnet.notify_off_mul = mul;
				dprintf("virtio-net: NOTIFY @ BAR%u+0x%x (mul=%u)\n", h.bar, h.offset, vnet.notify_off_mul);
			}
		}

		cap_ptr = (uint8_t) (next & 0xFC);
	}

	if (!vnet.common || !vnet.devcfg || !vnet.notify_base) {
		dprintf("virtio-net: missing required caps (COMMON/DEVICE/NOTIFY)\n");
		return false;
	}

	return true;
}

static inline void virtio_net_notify(uint16_t queue_id) {
	vnet.common->queue_select = queue_id;
	uint16_t notify_off = vnet.common->queue_notify_off;
	volatile uint16_t *doorbell = (volatile uint16_t *) (vnet.notify_base + (uint32_t) notify_off * vnet.notify_off_mul);
	*doorbell = queue_id;
}

static bool virtq_init(virtq_t *q, uint16_t q_index) {
	vnet.common->queue_select = q_index;
	uint16_t dev_qsz = vnet.common->queue_size;

	if (!dev_qsz) {
		dprintf("virtio-net: queue%u not available\n", q_index);
		return false;
	}

	uint16_t qsz = VNET_QSIZE;
	if (qsz > dev_qsz) {
		qsz = dev_qsz;
	}

	size_t desc_bytes = sizeof(virtq_desc_t) * qsz;
	size_t avail_bytes = sizeof(virtq_avail_t) + sizeof(uint16_t) * qsz;
	size_t used_bytes = sizeof(virtq_used_t) + sizeof(virtq_used_elem_t) * qsz;

	q->desc  = kmalloc_aligned(desc_bytes, 16);
	q->avail = kmalloc_aligned(avail_bytes, 16);
	q->used  = kmalloc_aligned(used_bytes, 16);

	if (!q->desc || !q->avail || !q->used) {
		kfree_aligned(q->desc);
		kfree_aligned(q->avail);
		kfree_aligned(q->used);
		dprintf("virtio-net: ring alloc failed\n");
		return false;
	}

	memset((void *) q->desc, 0, desc_bytes);
	memset((void *) q->avail, 0, avail_bytes);
	memset((void *) q->used, 0, used_bytes);

	q->q_size = qsz;
	q->avail_idx = 0;
	q->used_idx = 0;

	for (uint16_t i = 0; i < qsz - 1; i++) {
		q->desc[i].next = i + 1;
	}

	q->desc[qsz - 1].next = 0xFFFF;
	q->free_head = 0;

	vnet.common->queue_size = qsz;
	vnet.common->queue_desc = (uint64_t) (uintptr_t) q->desc;
	vnet.common->queue_driver = (uint64_t) (uintptr_t) q->avail;
	vnet.common->queue_device = (uint64_t) (uintptr_t) q->used;
	vnet.common->queue_enable = 1;

	q->notify_off = vnet.common->queue_notify_off;

	return true;
}

static int virtq_alloc_desc(virtq_t *q) {
	uint16_t h = q->free_head;
	if (h == 0xFFFF) {
		return -1;
	}
	q->free_head = q->desc[h].next;
	return h;
}

static void virtq_free_desc(virtq_t *q, uint16_t idx) {
	q->desc[idx].next = q->free_head;
	q->free_head = idx;
}

static inline void virtq_push_and_notify(virtq_t *q, uint16_t head_idx, uint16_t qid) {
	uint16_t mask = q->q_size - 1;

	q->avail->ring[q->avail_idx & mask] = head_idx;

	__asm__ volatile("sfence":: : "memory");

	q->avail->idx = q->avail_idx + 1;
	q->avail_idx = q->avail_idx + 1;

	virtio_net_notify(qid);
}

static int vnet_rx_prime_all(void) {
	virtq_t *rq = &vnet.rxq;

	for (uint16_t i = 0; i < rq->q_size; i++) {
		int d = virtq_alloc_desc(rq);
		if (d < 0) {
			return 0;
		}

		void* buf = kmalloc_aligned(VNET_RX_BUF_SIZE, 16);
		if (!buf) {
			return 0;
		}

		memset(buf, 0, VNET_RX_BUF_SIZE);

		vnet.rx_bufs[d] = buf;

		rq->desc[d].addr = (uint64_t) (uintptr_t) buf;
		rq->desc[d].len = VNET_RX_BUF_SIZE;
		rq->desc[d].flags = VIRTQ_DESC_F_WRITE;
		rq->desc[d].next = 0;

		virtq_push_and_notify(rq, (uint16_t) d, 0);
	}

	return 1;
}

static void vnet_rx_drain(void) {
	virtq_t *rq = &vnet.rxq;
	uint16_t mask = rq->q_size - 1;

	while (rq->used_idx != rq->used->idx) {
		virtq_used_elem_t e = rq->used->ring[rq->used_idx & mask];
		uint16_t head = (uint16_t) e.id;
		uint32_t len = e.len;
		uint8_t *buf = (uint8_t *) vnet.rx_bufs[head];

		/* RX buffer layout: [virtio_net_hdr][ethernet frame...] */
		size_t hdr_sz = sizeof(virtio_net_hdr_t);

		if (buf && len >= hdr_sz) {
			uint8_t *frame = buf + hdr_sz;
			uint16_t flen = (uint16_t) (len - hdr_sz);

			/* hand only the Ethernet frame to the stack */
			ethernet_handle_packet((ethernet_frame_t *) frame, flen);
		}

		/* repost the same buffer */
		rq->desc[head].addr = (uint64_t) (uintptr_t) buf;
		rq->desc[head].len = VNET_RX_BUF_SIZE;
		rq->desc[head].flags = VIRTQ_DESC_F_WRITE;
		rq->desc[head].next = 0;

		virtq_push_and_notify(rq, head, 0);

		rq->used_idx = rq->used_idx + 1;
	}
}

bool virtio_send_packet(void *data, uint16_t len) {
	if (len > 1518) {
		return false;
	}
	virtq_t* tq = &vnet.txq;

	int h = virtq_alloc_desc(tq);
	if (h < 0) {
		dprintf("virtio-net: TX ring full\n");
		return false;
	}
	int p = virtq_alloc_desc(tq);
	if (p < 0) {
		virtq_free_desc(tq, (uint16_t) h);
		dprintf("virtio-net: TX ring full\n");
		return false;
	}

	void* hdr = vnet.tx_hdrs[h];
	if (!hdr) {
		hdr = kmalloc_aligned(VNET_HDR_SIZE, 16);
		if (!hdr) {
			virtq_free_desc(tq, (uint16_t) h);
			virtq_free_desc(tq, (uint16_t) p);
			return false;
		}
		vnet.tx_hdrs[h] = hdr;
	}

	memset(hdr, 0, VNET_HDR_SIZE);

	void* pay = kmalloc_aligned(len, 16);
	if (!pay) {
		virtq_free_desc(tq, (uint16_t) h);
		virtq_free_desc(tq, (uint16_t) p);
		return false;
	}

	memcpy(pay, data, len);

	tq->desc[h].addr = (uint64_t) (uintptr_t) hdr;
	tq->desc[h].len = sizeof(virtio_net_hdr_t);
	tq->desc[h].flags = VIRTQ_DESC_F_NEXT;
	tq->desc[h].next = (uint16_t) p;

	tq->desc[p].addr = (uint64_t) (uintptr_t) pay;
	tq->desc[p].len = len;
	tq->desc[p].flags = 0;
	tq->desc[p].next = 0;

	virtq_push_and_notify(tq, (uint16_t) h, 1);

	return true;
}

static void vnet_tx_complete(void) {
	virtq_t* tq = &vnet.txq;
	uint16_t mask = tq->q_size - 1;

	while (tq->used_idx != tq->used->idx) {
		virtq_used_elem_t e = tq->used->ring[tq->used_idx & mask];
		uint16_t head = (uint16_t) e.id;
		uint16_t pay = tq->desc[head].next;

		void* paybuf = (void *) (uintptr_t) tq->desc[pay].addr;
		if (paybuf) {
			kfree_aligned(paybuf);
		}

		virtq_free_desc(tq, pay);
		virtq_free_desc(tq, head);
		tq->used_idx = tq->used_idx + 1;
	}
}

static void virtio_net_isr(uint8_t isr, uint64_t error, uint64_t irq, void *opaque) {
	[[maybe_unused]] volatile uint8_t cause = 0;
	if (vnet.isr) {
		/* acknowledge */
		cause = vnet.isr->isr;
	}
	vnet_rx_drain();
	vnet_tx_complete();
}

static void virtio_get_mac_addr(uint8_t *dst) {
	memcpy(dst, vnet.mac, 6);
}

static bool virtio_net_start(pci_dev_t *pdev) {
	vnet.pci = *pdev;
	vnet.device_id = pci_read(*pdev, PCI_DEVICE_ID);

	if (!virtio_net_hw_enable(*pdev)) {
		return false;
	}
	if (!virtio_net_probe_caps(*pdev)) {
		return false;
	}

	vnet.common->device_status = 0;
	vnet.common->device_status = VIRTIO_STATUS_ACKNOWLEDGE;
	vnet.common->device_status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;

	uint64_t devf = 0;
	vnet.common->device_feature_select = 0;
	devf |= vnet.common->device_feature;
	vnet.common->device_feature_select = 1;
	devf |= (uint64_t) vnet.common->device_feature << 32;

	uint64_t want = VIRTIO_F_VERSION_1;
	if (devf & VIRTIO_NET_F_MAC) {
		want |= VIRTIO_NET_F_MAC;
	}

	vnet.common->driver_feature_select = 0;
	vnet.common->driver_feature = (uint32_t) want;
	vnet.common->driver_feature_select = 1;
	vnet.common->driver_feature = (uint32_t) (want >> 32);

	vnet.common->device_status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK;

	if ((vnet.common->device_status & 8) == 0) {
		dprintf("virtio-net: FEATURES_OK not latched\n");
		vnet.common->device_status = 128;      /* FAILED */
		return false;
	}

	if (!virtq_init(&vnet.rxq, 0) || !virtq_init(&vnet.txq, 1)) {
		return false;
	}

	if (want & VIRTIO_NET_F_MAC) {
		memcpy(vnet.mac, (const void *) vnet.devcfg->mac, 6);
	} else {
		/* If we get here we are expected to create our own mac address randomly */
		uint64_t tsc = rdtsc();
		vnet.mac[0] = 0x02;
		vnet.mac[1] = (tsc >> 0) & 0xFF;
		vnet.mac[2] = (tsc >> 8) & 0xFF;
		vnet.mac[3] = (tsc >> 16) & 0xFF;
		vnet.mac[4] = (tsc >> 24) & 0xFF;
		vnet.mac[5] = (tsc >> 32) & 0xFF;
	}

	if (!vnet_rx_prime_all()) {
		return false;
	}

	vnet.common->device_status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK;
	pci_setup_interrupt("virtio-net", *pdev, logical_cpu_id(), virtio_net_isr, NULL);
	kprintf("virtio-net: MAC %02x:%02x:%02x:%02x:%02x:%02x\n", vnet.mac[0], vnet.mac[1], vnet.mac[2], vnet.mac[3], vnet.mac[4], vnet.mac[5]);

	netdev_t* net = kmalloc(sizeof(netdev_t));
	if (!net) {
		dprintf("virtio-net: OOM netdev\n");
		return false;
	}

	net->opaque = NULL;
	net->deviceid = (VIRTIO_VENDOR_ID << 16) | vnet.device_id;
	make_unique_device_name("net", net->name, sizeof(net->name));
	net->description = "virtio-net (modern)";
	net->flags = CONNECTED;
	net->mtu = 1500;
	net->netproto = NULL;
	net->num_netprotos = 0;
	net->speed = 1000;
	net->get_mac_addr = virtio_get_mac_addr;
	net->send_packet = virtio_send_packet;
	net->next = NULL;

	register_network_device(net);

	return true;
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("virtio-net: loaded\n");
	pci_dev_t dev = pci_get_device(VIRTIO_VENDOR_ID, VIRTIO_DEVICE_NET_MODERN, -1);
	if (!dev.bits) {
		dprintf("virtio-net: no modern virtio-net device\n");
		return false;
	}
	if (!virtio_net_start(&dev)) {
		return false;
	}
	network_setup();
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	return false;
}
