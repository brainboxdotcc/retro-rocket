#include <kernel.h>
#include "usb_core.h"
#include "usb_xhci.h"
#include "mmio.h"

static struct xhci_hc g_xhci; /* single controller */

/* ring helpers */
static void ring_init(struct xhci_ring *r, size_t bytes, uint64_t phys, void *virt) {
	r->base = (struct trb *) virt;
	r->phys = phys;
	r->num_trbs = (uint32_t) (bytes / sizeof(struct trb));
	r->enqueue = 0;
	r->cycle = 1; /* start with cycle=1 */
	memset(r->base, 0, bytes);

	/* terminal link TRB to form a ring */
	struct trb *link = &r->base[r->num_trbs - 1];
	link->lo = (uint32_t) (r->phys & 0xFFFFFFFFu);
	link->hi = (uint32_t) (r->phys >> 32);
	link->sts = 0;
	link->ctrl = TRB_SET_TYPE(TRB_LINK) | TRB_CYCLE;
}

/* enqueue a TRB (skips the terminal link slot) */
static struct trb *ring_push(struct xhci_ring *r) {
	if (r->enqueue == r->num_trbs - 1) {
		r->enqueue = 0;
		r->cycle ^= 1u;
	}
	struct trb *t = &r->base[r->enqueue++];
	memset(t, 0, sizeof(*t));
	return t;
}

static int xhci_cmd_submit_wait(struct xhci_hc *hc, struct trb *cmd_trb, uint64_t *out_cc_trb_lohi) {
	/* ring doorbell 0 (command) */
	mmio_write32(hc->db + XHCI_DOORBELL(0), 0);

	/* compute physical address of the command TRB we just posted */
	uint64_t cmd_phys = hc->cmd.phys +
			    (uint64_t)((uintptr_t)cmd_trb - (uintptr_t)hc->cmd.base);

	/* poll event ring for a matching command completion event (timeout ~50ms) */
	uint64_t deadline = get_ticks() + 50;
	for (;;) {
		for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
			struct trb *e = &hc->evt.base[i];
			if (e->ctrl == 0 && e->lo == 0 && e->hi == 0 && e->sts == 0) continue;

			uint32_t type = (e->ctrl >> 10) & 0x3Fu;
			if (type == TRB_CMD_COMPLETION) {
				uint64_t evt_ptr = ((uint64_t)e->hi << 32) | e->lo;
				if (evt_ptr == cmd_phys) {
					if (out_cc_trb_lohi) *out_cc_trb_lohi = evt_ptr;

					/* consume event and bump ERDP by one TRB (16B) */
					memset(e, 0, sizeof(*e));
					uint64_t erdp = mmio_read64(hc->rt + XHCI_RT_IR0 + IR_ERDP);
					erdp += 16;
					mmio_write64(hc->rt + XHCI_RT_IR0 + IR_ERDP, erdp);
					return 1;
				}
			}
		}
		if ((int64_t)(get_ticks() - deadline) > 0) break;
		__asm__ volatile("pause");
	}
	dprintf("xhci: command timeout\n");
	return 0;
}

static struct trb *xhci_cmd_begin(struct xhci_hc *hc) {
	struct trb *t = ring_push(&hc->cmd);
	t->ctrl |= hc->cmd.cycle ? TRB_CYCLE : 0;
	return t;
}

/* ---- controller init / reset ---- */
static int xhci_hw_enable_map(struct xhci_hc *hc, pci_dev_t dev) {
	uint32_t bar0 = pci_read(dev, PCI_BAR0);
	uint32_t bar1 = pci_read(dev, PCI_BAR1);
	if (!pci_bar_is_mem64(bar0)) {
		dprintf("xhci: BAR0 not 64-bit MMIO\n");
		return 0;
	}
	uint64_t base = pci_mem_base64(bar0, bar1);
	uint64_t size = get_bar_size(dev, 0);

	pci_enable_memspace(dev);
	pci_bus_master(dev);
	/*
	 * FIXME: This errors if the memory is already mapped. dont fail.
	 * if (!mmio_identity_map(base, size)) {
		dprintf("xhci: identity map failed\n");
		return 0;
	}*/

	hc->cap = (volatile uint8_t *)(uintptr_t) base;
	hc->cap_len   = (uint8_t) mmio_read32(hc->cap + XHCI_CAPLENGTH);
	uint32_t hcs1 = mmio_read32(hc->cap + XHCI_HCSPARAMS1);
	uint32_t hcs2 = mmio_read32(hc->cap + XHCI_HCSPARAMS2);
	uint32_t hcc1 = mmio_read32(hc->cap + XHCI_HCCPARAMS1);
	hc->op = hc->cap + hc->cap_len;
	hc->db = hc->cap + (mmio_read32(hc->cap + XHCI_DBOFF) & ~0x3u);
	hc->rt = hc->cap + (mmio_read32(hc->cap + XHCI_RTSOFF) & ~0x1Fu);

	hc->max_slots = (uint8_t)(hcs1 & 0xFFu);
	hc->port_count = (uint8_t)((hcs1 >> 24) & 0xFFu);
	hc->csz64 = (hcc1 & (1u << 2)) ? 1 : 0;

	/* DCBAA + scratchpads */
	uint32_t sp_lo = (hcs2 >> 27) & 0x1Fu;     /* Max Scratchpad Hi */
	uint32_t sp_hi = (hcs2 >> 21) & 0x1Fu;     /* Max Scratchpad Lo */
	uint32_t sp_count = (sp_lo << 5) | sp_hi;
	if (sp_count == 0) sp_count = 1;

	hc->dcbaa = (uint64_t *) kmalloc_aligned(4096, 4096);
	memset(hc->dcbaa, 0, 4096);
	hc->dcbaa_phys = (uint64_t)(uintptr_t) hc->dcbaa;

	hc->scratch_index = (uint64_t *) kmalloc_aligned(4096, 4096);
	memset(hc->scratch_index, 0, 4096);
	hc->scratch_index_phys = (uint64_t)(uintptr_t) hc->scratch_index;
	for (uint32_t i = 0; i < sp_count; i++) {
		void *pad = kmalloc_aligned(4096, 4096);
		memset(pad, 0, 4096);
		hc->scratch_index[i] = (uint64_t)(uintptr_t) pad;
	}
	hc->dcbaa[0] = hc->scratch_index_phys;

	dprintf("xhci: BAR0=0x%llx caplen=%u max_slots=%u ports=%u scratchpads=%u csz64=%u\n",
		(unsigned long long) base, hc->cap_len, hc->max_slots, hc->port_count, sp_count, hc->csz64);
	return 1;
}

static int xhci_reset_controller(struct xhci_hc *hc) {
	/* halt if running */
	uint32_t cmd = mmio_read32(hc->op + XHCI_USBCMD);
	if (cmd & USBCMD_RS) {
		mmio_write32(hc->op + XHCI_USBCMD, cmd & ~USBCMD_RS);
		uint64_t until = get_ticks() + 20; /* 20ms */
		while (((mmio_read32(hc->op + XHCI_USBSTS) & USBSTS_HCH) == 0) && get_ticks() < until) {
			__asm__ volatile("pause");
		}
	}

	/* HCRST */
	mmio_write32(hc->op + XHCI_USBCMD, USBCMD_HCRST);
	uint64_t until = get_ticks() + 100; /* 100ms */
	while ((mmio_read32(hc->op + XHCI_USBCMD) & USBCMD_HCRST) && get_ticks() < until) {
		__asm__ volatile("pause");
	}
	if (mmio_read32(hc->op + XHCI_USBSTS) & USBSTS_CNR) {
		dprintf("xhci: controller not ready after reset\n");
		return 0;
	}

	/* programme DCBAA */
	mmio_write64(hc->op + XHCI_DCBAP, hc->dcbaa_phys);

	/* command ring (4K) */
	void *cr_virt = kmalloc_aligned(4096, 4096);
	uint64_t cr_phys = (uint64_t)(uintptr_t) cr_virt;
	ring_init(&hc->cmd, 4096, cr_phys, cr_virt);
	mmio_write64(hc->op + XHCI_CRCR, (cr_phys & ~0xFu) | 1u); /* RCS=1 */

	/* event ring (4K) + 1-entry ERST */
	void *er_virt = kmalloc_aligned(4096, 4096);
	uint64_t er_phys = (uint64_t)(uintptr_t) er_virt;
	ring_init(&hc->evt, 4096, er_phys, er_virt);

	hc->erst = (struct erst_entry *) kmalloc_aligned(64, 64);
	memset(hc->erst, 0, 64);
	hc->erst[0].ring_base = er_phys;
	hc->erst[0].size = 256;
	hc->erst_phys = (uint64_t)(uintptr_t) hc->erst;

	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;
	mmio_write32(ir0 + IR_ERSTSZ, 1);
	mmio_write64(ir0 + IR_ERSTBA, hc->erst_phys);
	mmio_write64(ir0 + IR_ERDP, er_phys); /* dequeue at start */
	mmio_write32(ir0 + IR_IMOD, 64);
	mmio_write32(ir0 + IR_IMAN, IR_IMAN_IE); /* enable interrupter 0 */

	/* configure number of device slots */
	mmio_write32(hc->op + XHCI_CONFIG, hc->max_slots ? hc->max_slots : 8);

	/* start + enable interrupts */
	mmio_write32(hc->op + XHCI_USBCMD, USBCMD_RS | USBCMD_INTE);

	until = get_ticks() + 20; /* 20ms */
	while ((mmio_read32(hc->op + XHCI_USBSTS) & USBSTS_HCH) && get_ticks() < until) {
		__asm__ volatile("pause");
	}
	return 1;
}

/* ---- EP context helpers ---- */
static void ctx_program_ep0(struct ep_ctx *e, uint16_t mps, uint64_t tr_deq_phys) {
	e->dword0 = ((uint32_t)mps << 16) | (4u << 3) | (3u << 1); /* Control, CErr=3 */
	e->deq = (tr_deq_phys | 1u); /* DCS=1 */
	e->dword4 = 8u; /* avg TRB len */
}

static void ctx_program_ep1_in(struct ep_ctx *e, uint16_t mps, uint64_t tr_deq_phys, uint16_t esit_payload) {
	e->dword0 = ((uint32_t)mps << 16) | (7u << 3) | (3u << 1); /* Interrupt IN */
	e->deq = (tr_deq_phys | 1u);
	e->dword4 = ((uint32_t)esit_payload << 16) | (uint32_t)esit_payload;
}

/* ---- tiny inline control transfer used during enum ---- */
static int xhci_ctrl_build_and_run(struct xhci_hc *hc, uint8_t slot_id,
				   const uint8_t *setup8, void *data, uint16_t len, int dir_in)
{
	if (!hc->dev.ep0_tr.base) {
		void *v = kmalloc_aligned(4096, 4096);
		uint64_t p = (uint64_t)(uintptr_t) v;
		ring_init(&hc->dev.ep0_tr, 4096, p, v);
	}

	struct trb *t_setup = ring_push(&hc->dev.ep0_tr);
	uint64_t sphys = (uint64_t)(uintptr_t) setup8;
	t_setup->lo = (uint32_t)(sphys & 0xFFFFFFFFu);
	t_setup->hi = (uint32_t)(sphys >> 32);
	t_setup->sts = (3u << 16) | 8u;
	t_setup->ctrl = TRB_SET_TYPE(TRB_SETUP_STAGE) | TRB_IDT | (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);

	if (len) {
		struct trb *t_data = ring_push(&hc->dev.ep0_tr);
		uint64_t dphys = (uint64_t)(uintptr_t) data;
		t_data->lo = (uint32_t)(dphys & 0xFFFFFFFFu);
		t_data->hi = (uint32_t)(dphys >> 32);
		t_data->sts = len;
		t_data->ctrl = TRB_SET_TYPE(TRB_DATA_STAGE) |
			       (dir_in ? TRB_DIR : 0) |
			       (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);
	}

	struct trb *t_status = ring_push(&hc->dev.ep0_tr);
	t_status->lo = 0; t_status->hi = 0; t_status->sts = 0;
	t_status->ctrl = TRB_SET_TYPE(TRB_STATUS_STAGE) |
			 (dir_in ? 0 : TRB_DIR) | TRB_IOC |
			 (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);

	mmio_write32(hc->db + 4u * slot_id, EPID_CTRL);

	uint64_t until = get_ticks() + 50; /* 50ms */
	for (;;) {
		for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
			struct trb *e = &hc->evt.base[i];
			if (e->ctrl == 0 && e->lo == 0 && e->hi == 0 && e->sts == 0) continue;
			uint32_t type = (e->ctrl >> 10) & 0x3Fu;
			if (type == TRB_TRANSFER_EVENT) {
				memset(e, 0, sizeof(*e));
				uint64_t erdp = mmio_read64(hc->rt + XHCI_RT_IR0 + IR_ERDP);
				erdp += 16;
				mmio_write64(hc->rt + XHCI_RT_IR0 + IR_ERDP, erdp);
				return 1;
			}
		}
		if ((int64_t)(get_ticks() - until) > 0) break;
		__asm__ volatile("pause");
	}
	dprintf("xhci: ctrl xfer timeout\n");
	return 0;
}

/* simple wrapper for external users (hid etc.) */
int xhci_ctrl_xfer(struct usb_dev *ud, const uint8_t *setup,
		   void *data, uint16_t len, int data_dir_in)
{
	if (!ud || !ud->hc || !setup) return 0;
	struct xhci_hc *hc = (struct xhci_hc *) ud->hc;
	return xhci_ctrl_build_and_run(hc, ud->slot_id, setup, data, len, data_dir_in);
}

/* ---- minimal enumeration (one device) ---- */
static int xhci_enumerate_first_device(struct xhci_hc *hc, struct usb_dev *out_ud) {
	/* reset ports with a device present */
	for (uint8_t p = 0; p < hc->port_count; p++) {
		volatile uint8_t *pr = hc->op + XHCI_PORTREG_BASE + p * XHCI_PORT_STRIDE;
		uint32_t sc = mmio_read32(pr + PORTSC);
		if (sc & PORTSC_CCS) {
			mmio_write32(pr + PORTSC, sc | PORTSC_PR | PORTSC_WRC);
		}
	}
	/* wait ~100ms */
	{
		uint64_t end = get_ticks() + 100;
		while (get_ticks() < end) { __asm__ volatile("pause"); }
	}

	/* Enable Slot */
	struct trb *es = xhci_cmd_begin(hc);
	es->ctrl |= TRB_SET_TYPE(TRB_ENABLE_SLOT);
	if (!xhci_cmd_submit_wait(hc, es, NULL)) {
		return 0;
	}

	/* for v0, assume assigned slot_id = 1 */
	uint8_t slot_id = 1;
	hc->dev.slot_id = slot_id;

	/* allocate input context */
	hc->dev.ic = (struct input_ctx *) kmalloc_aligned(4096, 64);
	memset(hc->dev.ic, 0, sizeof(*hc->dev.ic));
	hc->dev.ic_phys = (uint64_t)(uintptr_t) hc->dev.ic;

	/* DCBAA entry for this slot -> device context (reuse ic memory in v0) */
	hc->dcbaa[slot_id] = hc->dev.ic_phys;

	/* EP0 transfer ring */
	void *tr0 = kmalloc_aligned(4096, 4096);
	uint64_t tr0p = (uint64_t)(uintptr_t) tr0;
	ring_init(&hc->dev.ep0_tr, 4096, tr0p, tr0);

	/* input context flags and fields */
	hc->dev.ic->add_flags = 0x00000003u; /* A0(slot) | A1(ep0) */
	hc->dev.ic->slot.dword0 = (1u << 27); /* 1 context entry */
	hc->dev.ic->slot.dword1 = (1u << 16); /* root port 1 */
	ctx_program_ep0(&hc->dev.ic->ep0, 8u, tr0p);

	/* Address Device */
	struct trb *ad = xhci_cmd_begin(hc);
	ad->lo = (uint32_t)(hc->dev.ic_phys & 0xFFFFFFFFu);
	ad->hi = (uint32_t)(hc->dev.ic_phys >> 32);
	ad->ctrl |= TRB_SET_TYPE(TRB_ADDRESS_DEVICE) | ((uint32_t)slot_id << 24);
	if (!xhci_cmd_submit_wait(hc, ad, NULL)) {
		return 0;
	}

	/* GET_DESCRIPTOR(Device, 8) */
	uint8_t setup_dev8[8] = { 0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 8, 0x00 };
	static uint8_t dev_desc[256] __attribute__((aligned(64)));
	memset(dev_desc, 0, sizeof(dev_desc));
	{
		struct trb *t_setup = ring_push(&hc->dev.ep0_tr);
		uint64_t sphys = (uint64_t)(uintptr_t) setup_dev8;
		t_setup->lo = (uint32_t)(sphys & 0xFFFFFFFFu);
		t_setup->hi = (uint32_t)(sphys >> 32);
		t_setup->sts = (3u << 16) | 8u;
		t_setup->ctrl = TRB_SET_TYPE(TRB_SETUP_STAGE) | TRB_IDT | (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);

		struct trb *t_data = ring_push(&hc->dev.ep0_tr);
		uint64_t dphys = (uint64_t)(uintptr_t) dev_desc;
		t_data->lo = (uint32_t)(dphys & 0xFFFFFFFFu);
		t_data->hi = (uint32_t)(dphys >> 32);
		t_data->sts = 8;
		t_data->ctrl = TRB_SET_TYPE(TRB_DATA_STAGE) | TRB_DIR | (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);

		struct trb *t_status = ring_push(&hc->dev.ep0_tr);
		t_status->lo = 0; t_status->hi = 0; t_status->sts = 0;
		t_status->ctrl = TRB_SET_TYPE(TRB_STATUS_STAGE) | TRB_IOC | (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);

		mmio_write32(hc->db + 4u * slot_id, EPID_CTRL);

		uint64_t until = get_ticks() + 50; /* 50ms */
		int got = 0;
		for (;;) {
			for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
				struct trb *e = &hc->evt.base[i];
				if (e->ctrl == 0 && e->lo == 0 && e->hi == 0 && e->sts == 0) continue;
				uint32_t type = (e->ctrl >> 10) & 0x3Fu;
				if (type == TRB_TRANSFER_EVENT) {
					memset(e, 0, sizeof(*e));
					uint64_t erdp = mmio_read64(hc->rt + XHCI_RT_IR0 + IR_ERDP);
					erdp += 16;
					mmio_write64(hc->rt + XHCI_RT_IR0 + IR_ERDP, erdp);
					got = 1;
					break;
				}
			}
			if (got) break;
			if ((int64_t)(get_ticks() - until) > 0) break;
			__asm__ volatile("pause");
		}
		if (!got) return 0;
	}

	uint16_t mps = dev_desc[7];
	if (mps != 8) {
		hc->dev.ic->add_flags = 0x00000002u; /* A1 only */
		ctx_program_ep0(&hc->dev.ic->ep0, mps, (uint64_t)(uintptr_t)hc->dev.ep0_tr.base);

		struct trb *ev = xhci_cmd_begin(hc);
		ev->lo = (uint32_t)(hc->dev.ic_phys & 0xFFFFFFFFu);
		ev->hi = (uint32_t)(hc->dev.ic_phys >> 32);
		ev->ctrl |= TRB_SET_TYPE(TRB_EVAL_CONTEXT) | ((uint32_t)slot_id << 24);
		if (!xhci_cmd_submit_wait(hc, ev, NULL)) {
			return 0;
		}
	}

	/* GET_DESCRIPTOR(Device, full) */
	memset(dev_desc, 0, sizeof(dev_desc));
	uint8_t setup_dev_full[8] = { 0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 18, 0x00 };
	if (!xhci_ctrl_xfer((struct usb_dev *)&(struct usb_dev){ .hc = hc, .slot_id = slot_id },
			    setup_dev_full, dev_desc, 18, 1)) {
		return 0;
	}

	/* GET_DESCRIPTOR(Configuration) short then full */
	static uint8_t cfg_buf[512] __attribute__((aligned(64)));
	uint8_t setup_cfg9[8] = { 0x80, 0x06, 0x00, 0x02, 0x00, 0x00, 9, 0x00 };
	if (!xhci_ctrl_xfer((struct usb_dev *)&(struct usb_dev){ .hc = hc, .slot_id = slot_id },
			    setup_cfg9, cfg_buf, 9, 1)) {
		return 0;
	}
	uint16_t total_len = (uint16_t) cfg_buf[2] | ((uint16_t)cfg_buf[3] << 8);
	if (total_len > sizeof(cfg_buf)) total_len = sizeof(cfg_buf);
	uint8_t setup_cfg_full[8] = { 0x80, 0x06, 0x00, 0x02, 0x00, 0x00,
				      (uint8_t)(total_len & 0xFFu), (uint8_t)(total_len >> 8) };
	if (!xhci_ctrl_xfer((struct usb_dev *)&(struct usb_dev){ .hc = hc, .slot_id = slot_id },
			    setup_cfg_full, cfg_buf, total_len, 1)) {
		return 0;
	}

	/* parse first interface descriptor */
	uint8_t dev_class = 0, dev_sub = 0, dev_proto = 0;
	for (uint16_t off = 9; off + 2 <= total_len; ) {
		uint8_t len = cfg_buf[off];
		uint8_t typ = cfg_buf[off + 1];
		if (len < 2) break;
		if (typ == 0x04 && len >= 9) {
			dev_class = cfg_buf[off + 5];
			dev_sub   = cfg_buf[off + 6];
			dev_proto = cfg_buf[off + 7];
			break;
		}
		off = (uint16_t) (off + len);
	}

	memset(out_ud, 0, sizeof(*out_ud));
	out_ud->hc = hc;
	out_ud->slot_id = slot_id;
	out_ud->address = 1; /* first address */
	out_ud->vid = (uint16_t)dev_desc[8] | ((uint16_t)dev_desc[9] << 8);
	out_ud->pid = (uint16_t)dev_desc[10] | ((uint16_t)dev_desc[11] << 8);
	out_ud->dev_class = dev_class;
	out_ud->dev_subclass = dev_sub;
	out_ud->dev_proto = dev_proto;
	out_ud->ep0.epid = EPID_CTRL;
	out_ud->ep0.mps = mps;
	out_ud->ep0.type = 0;
	out_ud->ep0.dir_in = 0;

	dprintf("xhci: device VID:PID=%04x:%04x class=%02x/%02x/%02x mps=%u\n",
		out_ud->vid, out_ud->pid, dev_class, dev_sub, dev_proto, mps);

	return 1;
}

/* ---- INTx ISR ---- */
static void xhci_isr(uint8_t isr, uint64_t error, uint64_t irq, void *opaque)
{
	(void)isr; (void)error; (void)irq;
	struct xhci_hc *hc = (struct xhci_hc *)opaque;
	if (!hc || !hc->cap) return;

	/* Acknowledge controller interrupt: clear USBSTS.EINT */
	uint32_t sts = mmio_read32(hc->op + XHCI_USBSTS);
	if (sts & USBSTS_EINT) {
		mmio_write32(hc->op + XHCI_USBSTS, sts);
	}

	/* Clear Interrupter 0 IP */
	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;
	uint32_t iman = mmio_read32(ir0 + IR_IMAN);
	if (iman & IR_IMAN_IP) {
		mmio_write32(ir0 + IR_IMAN, iman & ~IR_IMAN_IP);
	}

	/* Drain event ring */
	for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
		struct trb *e = &hc->evt.base[i];
		if (e->ctrl == 0 && e->lo == 0 && e->hi == 0 && e->sts == 0) continue;

		uint32_t type = (e->ctrl >> 10) & 0x3Fu;

		if (type == TRB_TRANSFER_EVENT) {
			/* Dispatch HID INT-IN completion if armed */
			if (hc->dev.int_cb && hc->dev.int_buf && hc->dev.int_pkt_len) {
				struct usb_dev ud = {0};
				ud.hc = hc;
				ud.slot_id = hc->dev.slot_id;
				hc->dev.int_cb(&ud, hc->dev.int_buf, hc->dev.int_pkt_len);

				/* Re-arm a Normal TRB */
				struct trb *n = ring_push(&hc->dev.int_in_tr);
				n->lo = (uint32_t)(hc->dev.int_buf_phys & 0xFFFFFFFFu);
				n->hi = (uint32_t)(hc->dev.int_buf_phys >> 32);
				n->sts = hc->dev.int_pkt_len;
				n->ctrl = TRB_SET_TYPE(TRB_NORMAL) | TRB_IOC |
					  (hc->dev.int_in_tr.cycle ? TRB_CYCLE : 0);

				/* Ring doorbell (EPID=3) */
				mmio_write32(hc->db + 4u * hc->dev.slot_id, EPID_EP1_IN);
			}
		}

		/* Clear consumed event and bump ERDP */
		memset(e, 0, sizeof(*e));
		uint64_t erdp = mmio_read64(ir0 + IR_ERDP);
		erdp += 16;
		mmio_write64(ir0 + IR_ERDP, erdp);
	}
}

/* ---- public interrupts-in arming ---- */
int xhci_arm_int_in(struct usb_dev *ud, uint16_t pkt_len, xhci_int_in_cb cb) {
	if (!ud || !ud->hc || !pkt_len || !cb) return 0;
	struct xhci_hc *hc = (struct xhci_hc *) ud->hc;

	if (!hc->dev.int_in_tr.base) {
		void *v = kmalloc_aligned(4096, 4096);
		uint64_t p = (uint64_t)(uintptr_t) v;
		ring_init(&hc->dev.int_in_tr, 4096, p, v);
	}

	if (!hc->dev.int_buf) {
		hc->dev.int_buf = (uint8_t *) kmalloc_aligned(pkt_len, 64);
		hc->dev.int_buf_phys = (uint64_t)(uintptr_t) hc->dev.int_buf;
	}
	hc->dev.int_cb = cb;
	hc->dev.int_pkt_len = pkt_len;

	/* Configure EP1 IN via Configure Endpoint */
	hc->dev.ic->add_flags = 0x00000009u; /* A0(slot) + A3(ep1 in) */
	ctx_program_ep1_in(&hc->dev.ic->ep1_in, 8u /* default */, hc->dev.int_in_tr.phys, pkt_len);

	struct trb *ce = xhci_cmd_begin(hc);
	ce->lo = (uint32_t)(hc->dev.ic_phys & 0xFFFFFFFFu);
	ce->hi = (uint32_t)(hc->dev.ic_phys >> 32);
	ce->ctrl |= TRB_SET_TYPE(TRB_CONFIGURE_EP) | ((uint32_t)ud->slot_id << 24);
	if (!xhci_cmd_submit_wait(hc, ce, NULL)) {
		return 0;
	}

	/* Post first Normal TRB; ISR will keep re-arming */
	struct trb *n = ring_push(&hc->dev.int_in_tr);
	n->lo = (uint32_t)(hc->dev.int_buf_phys & 0xFFFFFFFFu);
	n->hi = (uint32_t)(hc->dev.int_buf_phys >> 32);
	n->sts = hc->dev.int_pkt_len;
	n->ctrl = TRB_SET_TYPE(TRB_NORMAL) | TRB_IOC | (hc->dev.int_in_tr.cycle ? TRB_CYCLE : 0);

	mmio_write32(hc->db + 4u * ud->slot_id, EPID_EP1_IN);
	return 1;
}

/* ------------------------------------------------------------
 * Public entry: probe first xHCI controller and enumerate
 * ------------------------------------------------------------ */
int xhci_probe_and_init(uint8_t bus, uint8_t dev, uint8_t func) {
	pci_dev_t p = {0};
	p.bus_num = bus;
	p.device_num = dev;
	p.function_num = func;

	memset(&g_xhci, 0, sizeof(g_xhci));

	if (!xhci_hw_enable_map(&g_xhci, p)) return 0;
	if (!xhci_reset_controller(&g_xhci)) return 0;

	/* Legacy INTx (matches your rtl8139 path). MSI/MSI-X can be added later. */
	uint32_t irq_line = pci_read(p, PCI_INTERRUPT_LINE) & 0xFFu;
	uint32_t irq_pin  = pci_read(p, PCI_INTERRUPT_PIN) & 0xFFu;
	g_xhci.irq_line = (uint8_t) irq_line;
	g_xhci.irq_pin  = (uint8_t) irq_pin;

	register_interrupt_handler(IRQ_START + irq_line, xhci_isr, p, &g_xhci);
	dprintf("xhci: using legacy INTx IRQ=%u (PIN#%c)\n", irq_line, 'A' + (int)irq_pin - 1);

	struct usb_dev ud;
	if (!xhci_enumerate_first_device(&g_xhci, &ud)) {
		dprintf("xhci: controller up, 0 devices present\n");
		kprintf("USB xHCI: controller ready (interrupts on), published 0 devices\n");
		return 1; /* host is fine; no device yet */
	}

	usb_core_device_added(&ud);
	kprintf("USB xHCI: controller ready (interrupts on), published 1 device\n");
	return 1;
}

/* find first USB controller by class code 0x0C03 and ensure xHCI (prog-if 0x30) */
void init_usb_xhci(void) {
	/* match by class only, like your AHCI init uses 0x0106 */
	pci_dev_t usb = pci_get_device(0, 0, 0x0C03);
	if (!usb.bits) {
		dprintf("USB: no USB controllers found (class 0x0C03)\n");
		return;
	}

	/* check programming interface is xHCI (0x30) */
	uint32_t prog_if = pci_read(usb, PCI_PROG_IF);
	if (prog_if != 0x30) {
		dprintf("USB: controller at %u:%u.%u is not xHCI (prog-if %02x)\n",
			usb.bus_num, usb.device_num, usb.function_num, prog_if);
		return;
	}

	dprintf("USB: xHCI candidate at %u:%u.%u\n",
		usb.bus_num, usb.device_num, usb.function_num);

	/* one-and-done bring-up; xhci code handles BARs, interrupts, and enumeration */
	if (!xhci_probe_and_init(usb.bus_num, usb.device_num, usb.function_num)) {
		dprintf("USB: xHCI init failed at %u:%u.%u\n",
			usb.bus_num, usb.device_num, usb.function_num);
		return;
	}

	dprintf("USB: xHCI initialised at %u:%u.%u\n",
		usb.bus_num, usb.device_num, usb.function_num);
}