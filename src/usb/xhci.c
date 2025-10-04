#include <kernel.h>
#include "usb_core.h"
#include "usb_xhci.h"
#include "mmio.h"

static struct xhci_hc* g_xhci; /* single controller */

/* ring_init with debug */
static void ring_init(struct xhci_ring *r, size_t bytes, uint64_t phys, void *virt) {
	dprintf("xhci.dbg: ring_init bytes=%u phys=%llx virt=%llx\n",
		(unsigned)bytes, (unsigned long long)phys, (unsigned long long)(uintptr_t)virt);

	r->base = (struct trb *) virt;
	r->phys = phys;
	r->num_trbs = (uint32_t) (bytes / sizeof(struct trb));
	r->enqueue = 0;
	r->cycle = 1;
	memset(r->base, 0, bytes);

	/* terminal link TRB */
	struct trb *link = &r->base[r->num_trbs - 1];
	link->lo = (uint32_t) (r->phys & 0xFFFFFFFFu);
	link->hi = (uint32_t) (r->phys >> 32);
	link->sts = 0;
	link->ctrl = TRB_SET_TYPE(TRB_LINK) | TRB_CYCLE;

	dprintf("xhci.dbg:   TRBs=%u link_trb@%u lo=%08x hi=%08x ctrl=%08x\n",
		r->num_trbs, r->num_trbs - 1, link->lo, link->hi, link->ctrl);
}

/* ring_push with debug */
static struct trb *ring_push(struct xhci_ring *r) {
	uint32_t before = r->enqueue;
	if (r->enqueue == r->num_trbs - 1) {
		dprintf("xhci.dbg: ring_push wrap at idx=%u, flip cycle %u->%u\n",
			r->enqueue, r->cycle, r->cycle ^ 1u);
		r->enqueue = 0;
		r->cycle ^= 1u;
	}
	struct trb *t = &r->base[r->enqueue++];
	memset(t, 0, sizeof(*t));
	dprintf("xhci.dbg: ring_push from %u to %u, cycle=%u trb=%llx\n",
		before, r->enqueue, r->cycle, (unsigned long long)(uintptr_t)t);
	return t;
}

static int xhci_cmd_submit_wait(struct xhci_hc *hc, struct trb *cmd_trb, uint64_t *out_cc_trb_lohi) {
	(void)out_cc_trb_lohi;

	/* compute physical address of the TRB */
	uint64_t cmd_phys = hc->cmd.phys +
			    (uint64_t)((uintptr_t)cmd_trb - (uintptr_t)hc->cmd.base);

	dprintf("xhci.dbg: cmd_submit dbell0, trb_virt=%llx trb_phys=%llx ctrl=%08x\n",
		(unsigned long long)(uintptr_t)cmd_trb,
		(unsigned long long)cmd_phys, cmd_trb->ctrl);

	/* ring doorbell #0 (command) */
	mmio_write32(hc->db + 0, 0);

	/* set EHB before polling */
	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;
	uint64_t erdp0 = mmio_read64(ir0 + IR_ERDP);
	mmio_write64(ir0 + IR_ERDP, (erdp0 & ~0x7ull) | (1ull << 3));
	dprintf("xhci.dbg:   ERDP set, old=%llx new=%llx\n",
		(unsigned long long)erdp0,
		(unsigned long long)((erdp0 & ~0x7ull) | (1ull << 3)));

	/* poll event ring up to 50ms */
	uint64_t deadline = get_ticks() + 50;
	for (;;) {
		for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
			struct trb *e = &hc->evt.base[i];
			if (e->ctrl == 0 && e->lo == 0 && e->hi == 0 && e->sts == 0) continue;

			uint32_t type = (e->ctrl >> 10) & 0x3Fu;
			uint64_t evt_ptr = ((uint64_t)e->hi << 32) | e->lo;

			dprintf("xhci.dbg:   evt[%u] type=%u lo=%08x hi=%08x sts=%08x match=%s\n",
				i, type, e->lo, e->hi, e->sts, (evt_ptr == cmd_phys ? "yes" : "no"));

			if (type == TRB_CMD_COMPLETION && evt_ptr == cmd_phys) {
				/* bump ERDP and consume */
				memset(e, 0, sizeof(*e));
				uint64_t erdp = mmio_read64(ir0 + IR_ERDP);
				erdp = (erdp & ~0x7ull) + 16;
				mmio_write64(ir0 + IR_ERDP, erdp | (1ull << 3));
				dprintf("xhci.dbg:   cmd complete, ERDP=%llx\n", (unsigned long long)(erdp | (1ull << 3)));
				return 1;
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

static int xhci_hw_enable_map(struct xhci_hc *hc, pci_dev_t dev) {
	uint32_t bar0 = pci_read(dev, PCI_BAR0);
	uint32_t bar1 = pci_read(dev, PCI_BAR1);

	dprintf("xhci.dbg: hw_enable_map bus=%u dev=%u fn=%u BAR0=%08x BAR1=%08x\n",
		dev.bus_num, dev.device_num, dev.function_num, bar0, bar1);

	if (!pci_bar_is_mem64(bar0)) {
		dprintf("xhci: BAR0 not 64-bit MMIO\n");
		return 0;
	}

	uint64_t base = pci_mem_base64(bar0, bar1);
	uint64_t size = get_bar_size(dev, 0);

	dprintf("xhci.dbg:   mmio base=%llx size=%llx\n",
		(unsigned long long)base, (unsigned long long)size);

	pci_enable_memspace(dev);
	pci_bus_master(dev);

	hc->cap = (volatile uint8_t *)(uintptr_t) base;

	hc->cap_len = (uint8_t) mmio_read32(hc->cap + XHCI_CAPLENGTH);
	uint32_t hcs1 = mmio_read32(hc->cap + XHCI_HCSPARAMS1);
	uint32_t hcs2 = mmio_read32(hc->cap + XHCI_HCSPARAMS2);
	uint32_t hcc1 = mmio_read32(hc->cap + XHCI_HCCPARAMS1);

	hc->op = hc->cap + hc->cap_len;
	hc->db = hc->cap + (mmio_read32(hc->cap + XHCI_DBOFF) & ~0x3u);
	hc->rt = hc->cap + (mmio_read32(hc->cap + XHCI_RTSOFF) & ~0x1Fu);

	hc->max_slots  = (uint8_t)(hcs1 & 0xFFu);
	hc->port_count = (uint8_t)((hcs1 >> 24) & 0xFFu);
	hc->csz64      = (hcc1 & (1u << 2)) ? 1 : 0;

	uint32_t sp_lo = (hcs2 >> 27) & 0x1Fu;
	uint32_t sp_hi = (hcs2 >> 21) & 0x1Fu;
	uint32_t sp_count = (sp_lo << 5) | sp_hi;
	if (sp_count == 0) sp_count = 1;

	dprintf("xhci.dbg:   cap_len=%u hcs1=%08x hcs2=%08x hcc1=%08x\n", hc->cap_len, hcs1, hcs2, hcc1);
	dprintf("xhci.dbg:   op=%llx db=%llx rt=%llx max_slots=%u ports=%u csz64=%u scratchpads=%u\n",
		(unsigned long long)(uintptr_t)hc->op,
		(unsigned long long)(uintptr_t)hc->db,
		(unsigned long long)(uintptr_t)hc->rt,
		hc->max_slots, hc->port_count, hc->csz64, sp_count);

	/* DCBAA + scratchpads */
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
	dprintf("xhci.dbg: reset_controller enter\n");

	uint32_t cmd = mmio_read32(hc->op + XHCI_USBCMD);
	uint32_t sts0 = mmio_read32(hc->op + XHCI_USBSTS);
	dprintf("xhci.dbg:   USBCMD=%08x USBSTS=%08x\n", cmd, sts0);

	/* halt if running */
	if (cmd & USBCMD_RS) {
		mmio_write32(hc->op + XHCI_USBCMD, cmd & ~USBCMD_RS);
		uint64_t until = get_ticks() + 20;
		while (((mmio_read32(hc->op + XHCI_USBSTS) & USBSTS_HCH) == 0) && get_ticks() < until) {
			__asm__ volatile("pause");
		}
		dprintf("xhci.dbg:   halted, USBSTS=%08x\n", mmio_read32(hc->op + XHCI_USBSTS));
	}

	/* HCRST */
	mmio_write32(hc->op + XHCI_USBCMD, USBCMD_HCRST);
	uint64_t until = get_ticks() + 100;
	while ((mmio_read32(hc->op + XHCI_USBCMD) & USBCMD_HCRST) && get_ticks() < until) {
		__asm__ volatile("pause");
	}
	uint32_t sts1 = mmio_read32(hc->op + XHCI_USBSTS);
	dprintf("xhci.dbg:   post HCRST USBSTS=%08x\n", sts1);
	if (sts1 & USBSTS_CNR) {
		dprintf("xhci: controller not ready after reset\n");
		return 0;
	}

	/* clear pending status */
	mmio_write32(hc->op + XHCI_USBSTS, sts1);

	/* DCBAA */
	mmio_write64(hc->op + XHCI_DCBAP, hc->dcbaa_phys);
	dprintf("xhci.dbg:   DCBAP=%llx\n", (unsigned long long)hc->dcbaa_phys);

	/* command ring */
	void *cr_virt = kmalloc_aligned(4096, 4096);
	uint64_t cr_phys = (uint64_t)(uintptr_t) cr_virt;
	ring_init(&hc->cmd, 4096, cr_phys, cr_virt);
	mmio_write64(hc->op + XHCI_CRCR, (cr_phys & ~0xFu) | 1u);
	dprintf("xhci.dbg:   CRCR=%llx\n", (unsigned long long)((cr_phys & ~0xFu) | 1u));

	/* event ring (linear buffer, not link ring) */
	void *er_virt = kmalloc_aligned(4096, 4096);
	uint64_t er_phys = (uint64_t)(uintptr_t) er_virt;
	memset(er_virt, 0, 4096);
	hc->evt.base = (struct trb *) er_virt;
	hc->evt.phys = er_phys;
	hc->evt.num_trbs = (uint32_t)(4096 / sizeof(struct trb));
	hc->evt.enqueue = 0;
	hc->evt.cycle = 1;

	hc->erst = (struct erst_entry *) kmalloc_aligned(64, 64);
	memset(hc->erst, 0, 64);
	hc->erst[0].ring_base = er_phys;
	hc->erst[0].size = 256;
	hc->erst_phys = (uint64_t)(uintptr_t) hc->erst;

	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;
	mmio_write32(ir0 + IR_ERSTSZ, 1);
	mmio_write64(ir0 + IR_ERSTBA, hc->erst_phys);
	mmio_write64(ir0 + IR_ERDP, er_phys | (1ull << 3));
	mmio_write32(ir0 + IR_IMOD, 64);
	mmio_write32(ir0 + IR_IMAN, IR_IMAN_IE);

	dprintf("xhci.dbg:   ERSTSZ=1 ERSTBA=%llx ERDP=%llx IMOD=64 IMAN=%08x\n",
		(unsigned long long)hc->erst_phys,
		(unsigned long long)(er_phys | (1ull << 3)),
		mmio_read32(ir0 + IR_IMAN));

	/* CONFIG, RS+INTE */
	mmio_write32(hc->op + XHCI_CONFIG, hc->max_slots ? hc->max_slots : 8);
	mmio_write32(hc->op + XHCI_USBCMD, USBCMD_RS | USBCMD_INTE);

	until = get_ticks() + 20;
	while ((mmio_read32(hc->op + XHCI_USBSTS) & USBSTS_HCH) && get_ticks() < until) {
		__asm__ volatile("pause");
	}

	uint32_t sts2 = mmio_read32(hc->op + XHCI_USBSTS);
	uint32_t cmd2 = mmio_read32(hc->op + XHCI_USBCMD);
	dprintf("xhci.dbg:   after start USBCMD=%08x USBSTS=%08x\n", cmd2, sts2);
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

/* ---- tiny inline control transfer used during enum (debug/verbose) ---- */
static int xhci_ctrl_build_and_run(struct xhci_hc *hc, uint8_t slot_id,
				   const uint8_t *setup8, void *data, uint16_t len, int dir_in)
{
	if (!hc || !setup8 || slot_id == 0) return 0;

	if (!hc->dev.ep0_tr.base) {
		void *v = kmalloc_aligned(4096, 4096);
		uint64_t p = (uint64_t)(uintptr_t) v;
		if (!v) {
			dprintf("xhci.dbg: ep0_tr alloc failed\n");
			return 0;
		}
		ring_init(&hc->dev.ep0_tr, 4096, p, v);
	}

	/* Build SETUP TRB with IDT: the 8 bytes must be EMBEDDED, not a pointer. */
	struct trb *t_setup = ring_push(&hc->dev.ep0_tr);
	/* embed the 8 bytes directly into lo/hi */
	uint32_t setup_lo = *(const uint32_t *)&setup8[0];
	uint32_t setup_hi = *(const uint32_t *)&setup8[4];
	t_setup->lo = setup_lo;
	t_setup->hi = setup_hi;

	/* TRT field (bits 17:16 of sts): 0=no data, 2=IN data, 3=OUT data */
	uint32_t trt = 0;
	if (len) trt = dir_in ? 2u : 3u;

	t_setup->sts  = (trt << 16) | 8u; /* 8 bytes of setup */
	t_setup->ctrl = TRB_SET_TYPE(TRB_SETUP_STAGE) | TRB_IDT |
			(hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);

	dprintf("xhci.dbg:   setup TRB @%08x ctrl=%08x lo=%08x hi=%08x sts=%08x (TRT=%u, len=%u, dir=%s)\n",
		(uint32_t)(uintptr_t)t_setup, t_setup->ctrl, t_setup->lo, t_setup->hi, t_setup->sts,
		trt, (unsigned)len, dir_in ? "IN" : "OUT");

	/* ----- DATA STAGE: use ctrl_dma bounce buffer if there is a payload ----- */
	int used_bounce = 0;
	if (len) {
		/* ensure bounce buffer exists & is large enough */
		if (!hc->dev.ctrl_dma || hc->dev.ctrl_dma_sz < len) {
			if (hc->dev.ctrl_dma) {
				kfree_null(&hc->dev.ctrl_dma);
				hc->dev.ctrl_dma_phys = 0;
				hc->dev.ctrl_dma_sz = 0;
			}
			hc->dev.ctrl_dma_sz = len;
			hc->dev.ctrl_dma = (uint8_t *) kmalloc_aligned(hc->dev.ctrl_dma_sz, 64);
			if (!hc->dev.ctrl_dma) {
				dprintf("xhci.dbg:   ctrl_dma alloc failed (len=%u)\n", (unsigned)len);
				return 0;
			}
			hc->dev.ctrl_dma_phys = (uint64_t)(uintptr_t) hc->dev.ctrl_dma;
			dprintf("xhci.dbg:   ctrl_dma=%p phys=%llx sz=%u\n",
				hc->dev.ctrl_dma, (unsigned long long)hc->dev.ctrl_dma_phys, (unsigned)hc->dev.ctrl_dma_sz);
		}

		/* For OUT: copy caller buffer -> DMA bounce */
		if (!dir_in) {
			memcpy(hc->dev.ctrl_dma, data, len);
		}
		used_bounce = 1;

		struct trb *t_data = ring_push(&hc->dev.ep0_tr);
		uint64_t dphys = hc->dev.ctrl_dma_phys;
		t_data->lo = (uint32_t)(dphys & 0xFFFFFFFFu);
		t_data->hi = (uint32_t)(dphys >> 32);
		t_data->sts = len;
		t_data->ctrl = TRB_SET_TYPE(TRB_DATA_STAGE) |
			       (dir_in ? TRB_DIR : 0) |
			       (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);
		dprintf("xhci.dbg:   data  TRB @%08x ctrl=%08x dphys=%p lo=%08x hi=%08x sts=%08x (bounce=%d)\n",
			(uint32_t)(uintptr_t)t_data, t_data->ctrl, (void*)dphys, t_data->lo, t_data->hi, t_data->sts, used_bounce);
	}

	struct trb *t_status = ring_push(&hc->dev.ep0_tr);
	t_status->lo = 0; t_status->hi = 0; t_status->sts = 0;
	/* Status direction is opposite of data stage (or IN if no data stage). */
	int status_in = (len == 0) ? 1 : (!dir_in);
	t_status->ctrl = TRB_SET_TYPE(TRB_STATUS_STAGE) |
			 (status_in ? TRB_DIR : 0) | TRB_IOC |
			 (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);

	dprintf("xhci.dbg:   stat  TRB @%08x ctrl=%08x\n",
		(uint32_t)(uintptr_t)t_status, t_status->ctrl);

	/* Ring doorbell (EP0 = 1) */
	mmio_write32(hc->db + 4u * slot_id, EPID_CTRL);
	dprintf("xhci.dbg:   ring doorbell EP0 (slot=%u)\n", slot_id);

	/* Poll event ring for a transfer event */
	uint64_t until = get_ticks() + 50; /* ~50ms */
	for (;;) {
		for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
			struct trb *e = &hc->evt.base[i];
			if (e->ctrl == 0 && e->lo == 0 && e->hi == 0 && e->sts == 0) continue;
			uint32_t type = (e->ctrl >> 10) & 0x3Fu;
			dprintf("xhci.dbg:   evt[%u] type=%u lo=%08x hi=%08x sts=%08x\n",
				i, type, e->lo, e->hi, e->sts);

			if (type == TRB_TRANSFER_EVENT) {
				/* If IN transfer and we used bounce, copy back to caller buffer now */
				if (len && dir_in && used_bounce) {
					memcpy(data, hc->dev.ctrl_dma, len);
				}
				/* Consume event and bump ERDP */
				memset(e, 0, sizeof(*e));
				uint64_t erdp = mmio_read64(hc->rt + XHCI_RT_IR0 + IR_ERDP);
				mmio_write64(hc->rt + XHCI_RT_IR0 + IR_ERDP, erdp);
				return 1;
			}
		}
		if ((int64_t)(get_ticks() - until) > 0) break;
		__asm__ volatile("pause");
	}
	dprintf("xhci.dbg:   ctrl xfer timeout/fail\n");
	return 0;
}

/* simple wrapper for external users (hid etc.) */
int xhci_ctrl_xfer(struct usb_dev *ud, const uint8_t *setup,
		   void *data, uint16_t len, int data_dir_in)
{
	if (!ud || !ud->hc || !setup) return 0;
	struct xhci_hc *hc = (struct xhci_hc *) ud->hc;
	dprintf("HERE 2\n");
	return xhci_ctrl_build_and_run(hc, ud->slot_id, setup, data, len, data_dir_in);
}

/* ---- minimal enumeration (one device) — debug/verbose, fixed SETUP handling ---- */
static int xhci_enumerate_first_device(struct xhci_hc *hc, struct usb_dev *out_ud) {
	if (!hc || !out_ud) return 0;

	dprintf("xhci.dbg: enumerate_first_device ports=%u\n", hc->port_count);

	/* Find a connected port and reset it; remember which port the device is on. */
	uint8_t attached_port = 0;
	for (uint8_t p = 0; p < hc->port_count; p++) {
		volatile uint8_t *pr = hc->op + XHCI_PORTREG_BASE + p * XHCI_PORT_STRIDE;
		uint32_t sc = mmio_read32(pr + PORTSC);
		dprintf("xhci.dbg:   PORT%u PORTSC=%08x\n", p + 1, sc);

		if (sc & PORTSC_CCS) {
			attached_port = p + 1;
			dprintf("xhci.dbg:   reset PORT%u\n", attached_port);
			/* write-1-to-clear + set PR */
			mmio_write32(pr + PORTSC, (sc & PORTSC_RW1C_MASK) | PORTSC_PR);
		}
	}
	if (!attached_port) {
		dprintf("xhci.dbg:   no connected ports\n");
		return 0;
	}

	/* ~100ms settle */
	uint64_t end_wait = get_ticks() + 100;
	while (get_ticks() < end_wait) { __asm__ volatile("pause"); }

	/* Enable Slot */
	struct trb *es = xhci_cmd_begin(hc);
	es->ctrl |= TRB_SET_TYPE(TRB_ENABLE_SLOT);
	dprintf("xhci.dbg: ring_push for ENABLE_SLOT trb=%08x ctrl=%08x\n",
		(uint32_t)(uintptr_t)es, es->ctrl);
	if (!xhci_cmd_submit_wait(hc, es, NULL)) return 0;
	dprintf("xhci.dbg:   ENABLE_SLOT ok\n");

	/* For simplicity, assume slot 1. */
	uint8_t slot_id = 1;

	/* Allocate and clear Input Context */
	if (!hc->dev.ic) {
		hc->dev.ic = (struct input_ctx *) kmalloc_aligned(4096, 64);
		if (!hc->dev.ic) {
			dprintf("xhci.dbg:   input ctx alloc failed\n");
			return 0;
		}
		hc->dev.ic_phys = (uint64_t)(uintptr_t) hc->dev.ic;
	}
	memset(hc->dev.ic, 0, 4096);

	/* Allocate a separate Device Context page and point DCBAA[slot] at it. */
	void *devctx = kmalloc_aligned(4096, 64);
	if (!devctx) {
		dprintf("xhci.dbg:   device ctx alloc failed\n");
		return 0;
	}
	memset(devctx, 0, 4096);
	uint64_t devctx_phys = (uint64_t)(uintptr_t)devctx;
	hc->dcbaa[slot_id] = devctx_phys;
	dprintf("xhci.dbg:   DCBAA[%u]=%x (device ctx phys)\n", slot_id, (uint32_t)devctx_phys);

	/* EP0 transfer ring (4K) */
	if (!hc->dev.ep0_tr.base) {
		void *tr0 = kmalloc_aligned(4096, 4096);
		if (!tr0) {
			dprintf("xhci.dbg:   ep0 ring alloc failed\n");
			kfree_null(&devctx);
			return 0;
		}
		uint64_t tr0p = (uint64_t)(uintptr_t) tr0;
		ring_init(&hc->dev.ep0_tr, 4096, tr0p, tr0);
	}

	/* Fill Input Context: A0 (slot), A1 (EP0), route=0, root port=attached_port, speed from PORTSC. */
	volatile uint8_t *pr_att = hc->op + XHCI_PORTREG_BASE + (attached_port - 1u) * XHCI_PORT_STRIDE;
	uint32_t psc = mmio_read32(pr_att + PORTSC);
	uint32_t speed = (psc >> 10) & 0xFu; /* 2.0/3.x speed code */

	hc->dev.ic->add_flags      = 0x00000003u;    /* A0|A1 */
	hc->dev.ic->slot.dword0    = (1u << 27);     /* 2 contexts => value 1 in CEC (EP0) */
	hc->dev.ic->slot.dword1    = ((uint32_t)attached_port << 16); /* Root hub port number */
	hc->dev.ic->slot.dword2    = speed;          /* speed in bits [3:0] */
	/* Program EP0 with default MPS 8 (per xHCI default address state) */
	ctx_program_ep0(&hc->dev.ic->ep0, 8u, hc->dev.ep0_tr.phys);

	/* Address Device with BSR=1 (don't assign USB address yet). */
	struct trb *ad = xhci_cmd_begin(hc);
	ad->lo = (uint32_t)(hc->dev.ic_phys & 0xFFFFFFFFu);
	ad->hi = (uint32_t)(hc->dev.ic_phys >> 32);
	ad->ctrl |= TRB_SET_TYPE(TRB_ADDRESS_DEVICE) | ((uint32_t)slot_id << 24) | (1u << 9); /* BSR=1 */
	dprintf("xhci.dbg: ring_push for ADDRESS_DEVICE trb=%08x ic_phys=%08x\n",
		(uint32_t)(uintptr_t)ad, (uint32_t)hc->dev.ic_phys);
	if (!xhci_cmd_submit_wait(hc, ad, NULL)) {
		kfree_null(&devctx);
		return 0;
	}
	dprintf("xhci.dbg:   ADDRESS_DEVICE ok\n");

	/* (Optional) Read PORTSC again to observe state */
	{
		uint32_t sc = mmio_read32(pr_att + PORTSC);
		dprintf("xhci.dbg:   PORTSC after address=%08x\n", sc);
	}

	/* === GET_DESCRIPTOR(Device, first 8 bytes) === */
	uint8_t  __attribute__((aligned(64))) setup_dev8[8] = { 0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 8, 0x00 };
	uint8_t  __attribute__((aligned(64))) dev_desc[256];
	memset(dev_desc, 0, sizeof(dev_desc));

	dprintf("HERE 1 setup8_dev=%p dev_desc=%p\n", &setup_dev8, &dev_desc);
	if (!xhci_ctrl_build_and_run(hc, slot_id, setup_dev8, dev_desc, 8, 1)) {
		dprintf("xhci.dbg:   dev transfer timeout (GET_DESCRIPTOR 8)\n");
		kfree_null(&devctx);
		return 0;
	}

	/* If MPS != 8, update EP0 to the reported MPS and Evaluate Context. */
	uint16_t mps = dev_desc[7];
	if (mps != 8) {
		hc->dev.ic->add_flags = 0x00000002u; /* A1 only */
		ctx_program_ep0(&hc->dev.ic->ep0, mps, hc->dev.ep0_tr.phys);

		struct trb *ev = xhci_cmd_begin(hc);
		ev->lo = (uint32_t)(hc->dev.ic_phys & 0xFFFFFFFFu);
		ev->hi = (uint32_t)(hc->dev.ic_phys >> 32);
		ev->ctrl |= TRB_SET_TYPE(TRB_EVAL_CONTEXT) | ((uint32_t)slot_id << 24);
		dprintf("xhci.dbg:   sending EVAL_CONTEXT slot=%u trb=%08x\n",
			slot_id, (uint32_t)(uintptr_t)ev);
		if (!xhci_cmd_submit_wait(hc, ev, NULL)) {
			kfree_null(&devctx);
			return 0;
		}
	}

	/* === GET_DESCRIPTOR(Device, full 18) === */
	uint8_t  __attribute__((aligned(64))) setup_dev_full[8] = { 0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 18, 0x00 };
	dprintf("setup_dev_full=%p HERE 2\n", &setup_dev_full);
	if (!xhci_ctrl_build_and_run(hc, slot_id, setup_dev_full, dev_desc, 18, 1)) {
		kfree_null(&devctx);
		return 0;
	}

	/* === GET_DESCRIPTOR(Configuration) short then full === */
	uint8_t cfg_buf[512] __attribute__((aligned(64)));
	uint8_t  __attribute__((aligned(64))) setup_cfg9[8] = { 0x80, 0x06, 0x00, 0x02, 0x00, 0x00, 9, 0x00 };
	dprintf("setup_cfg9=%p, cfg_buf=%p HERE 3\n", &setup_cfg9, &cfg_buf);
	if (!xhci_ctrl_build_and_run(hc, slot_id, setup_cfg9, cfg_buf, 9, 1)) {
		kfree_null(&devctx);
		return 0;
	}
	uint16_t total_len = (uint16_t) cfg_buf[2] | ((uint16_t)cfg_buf[3] << 8);
	if (total_len > sizeof(cfg_buf)) total_len = sizeof(cfg_buf);
	uint8_t setup_cfg_full[8] = {
		0x80, 0x06, 0x00, 0x02, 0x00, 0x00,
		(uint8_t)(total_len & 0xFFu), (uint8_t)(total_len >> 8)
	};
	dprintf("setup_cfg_full=%p HERE 4\n", &setup_cfg_full);
	if (!xhci_ctrl_build_and_run(hc, slot_id, setup_cfg_full, cfg_buf, total_len, 1)) {
		kfree_null(&devctx);
		return 0;
	}

	/* Parse first interface descriptor */
	uint8_t dev_class = 0, dev_sub = 0, dev_proto = 0;
	for (uint16_t off = 9; off + 2 <= total_len; ) {
		uint8_t len = cfg_buf[off];
		uint8_t typ = cfg_buf[off + 1];
		if (len < 2) break;
		if (typ == 0x04 && len >= 9) {
			dev_class = cfg_buf[off + 5];
			dev_sub   = cfg_buf[off + 6];
			dev_proto = cfg_buf[off + 7];
			dprintf("GOT HERE\n");
			break;
		}
		off = (uint16_t)(off + len);
	}

	/* Fill out usb_dev */
	memset(out_ud, 0, sizeof(*out_ud));
	out_ud->hc          = hc;
	out_ud->slot_id     = slot_id;
	out_ud->address     = 1; /* first address */
	out_ud->vid         = (uint16_t)dev_desc[8]  | ((uint16_t)dev_desc[9]  << 8);
	out_ud->pid         = (uint16_t)dev_desc[10] | ((uint16_t)dev_desc[11] << 8);
	out_ud->dev_class   = dev_class;
	out_ud->dev_subclass= dev_sub;
	out_ud->dev_proto   = dev_proto;
	out_ud->ep0.epid    = EPID_CTRL;
	out_ud->ep0.mps     = mps;
	out_ud->ep0.type    = 0;
	out_ud->ep0.dir_in  = 0;

	dprintf("xhci.dbg:   device VID:PID=%04x:%04x class=%02x/%02x/%02x mps=%u (port=%u speed=%u)\n",
		out_ud->vid, out_ud->pid, dev_class, dev_sub, dev_proto, mps, attached_port, speed);

	/* NOTE: devctx is intentionally not freed here; it’s now owned by HW via DCBAA[slot]. */
	hc->dev.slot_id = slot_id;
	return 1;
}

static void xhci_isr(uint8_t isr, uint64_t error, uint64_t irq, void *opaque)
{
	(void)isr; (void)error; (void)irq;
	struct xhci_hc *hc = (struct xhci_hc *)opaque;
	if (!hc || !hc->cap) return;

	uint32_t sts = mmio_read32(hc->op + XHCI_USBSTS);
	if (sts & USBSTS_EINT) {
		mmio_write32(hc->op + XHCI_USBSTS, sts);
	}
	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;
	uint32_t iman = mmio_read32(ir0 + IR_IMAN);
	if (iman & IR_IMAN_IP) {
		mmio_write32(ir0 + IR_IMAN, iman & ~IR_IMAN_IP);
	}

	for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
		struct trb *e = &hc->evt.base[i];
		if (e->ctrl == 0 && e->lo == 0 && e->hi == 0 && e->sts == 0) continue;

		uint32_t type = (e->ctrl >> 10) & 0x3Fu;
		dprintf("xhci.dbg: ISR evt[%u] type=%u lo=%08x hi=%08x sts=%08x\n",
			i, type, e->lo, e->hi, e->sts);

		if (type == TRB_TRANSFER_EVENT) {
			if (hc->dev.int_cb && hc->dev.int_buf && hc->dev.int_pkt_len) {
				struct usb_dev ud = {0};
				ud.hc = hc;
				ud.slot_id = hc->dev.slot_id;
				hc->dev.int_cb(&ud, hc->dev.int_buf, hc->dev.int_pkt_len);

				struct trb *n = ring_push(&hc->dev.int_in_tr);
				n->lo = (uint32_t)(hc->dev.int_buf_phys & 0xFFFFFFFFu);
				n->hi = (uint32_t)(hc->dev.int_buf_phys >> 32);
				n->sts = hc->dev.int_pkt_len;
				n->ctrl = TRB_SET_TYPE(TRB_NORMAL) | TRB_IOC |
					  (hc->dev.int_in_tr.cycle ? TRB_CYCLE : 0);

				mmio_write32(hc->db + 4u * hc->dev.slot_id, EPID_EP1_IN);
			}
		}

		memset(e, 0, sizeof(*e));
		uint64_t erdp = mmio_read64(ir0 + IR_ERDP);
		erdp = (erdp & ~0x7ull) + 16;
		mmio_write64(ir0 + IR_ERDP, erdp | (1ull << 3));
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

	g_xhci = kmalloc_aligned(sizeof(*g_xhci), 4096);

	memset(g_xhci, 0, sizeof(*g_xhci));

	if (!xhci_hw_enable_map(g_xhci, p)) {
		kfree_null(&g_xhci);
		return 0;
	}
	if (!xhci_reset_controller(g_xhci)) {
		kfree_null(&g_xhci);
		return 0;
	}

	/* Legacy INTx (matches your rtl8139 path). MSI/MSI-X can be added later. */
	uint32_t irq_line = pci_read(p, PCI_INTERRUPT_LINE) & 0xFFu;
	uint32_t irq_pin  = pci_read(p, PCI_INTERRUPT_PIN) & 0xFFu;
	g_xhci->irq_line = (uint8_t) irq_line;
	g_xhci->irq_pin  = (uint8_t) irq_pin;

	register_interrupt_handler(IRQ_START + irq_line, xhci_isr, p, g_xhci);
	dprintf("xhci: using legacy INTx IRQ=%u (PIN#%c)\n", irq_line, 'A' + (int)irq_pin - 1);

	struct usb_dev ud;
	if (!xhci_enumerate_first_device(g_xhci, &ud)) {
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