#include <kernel.h>
#include "usb_core.h"
#include "usb_xhci.h"
#include "mmio.h"

struct xhci_hc *g_xhci = NULL; /* single controller */

static void xhci_disable_interrupts(struct xhci_hc *hc);

static void xhci_deliver_ep1in(struct xhci_hc *hc, const struct trb *ev, bool deliver_to_driver);

static inline uint8_t trb_get_type(const struct trb *e) {
	return (uint8_t) ((e->ctrl >> 10) & 0x3F);
}

static inline uint8_t trb_get_epid(const struct trb *e) {
	return (uint8_t) ((e->ctrl >> 16) & 0x1F);
}

static inline uint8_t trb_get_cc(const struct trb *e) {
	return (uint8_t) ((e->sts >> 24) & 0xFF);
}

static inline uint32_t trb_get_bytes_remaining(const struct trb *e) {
	return (e->sts & 0x00FFFFFFu);
}

static inline uint8_t trb_get_slotid(const struct trb *e) {
	/* SlotID is bits [7:0] of control dword per xHCI spec */
	return (uint8_t) (e->ctrl & 0xFF);
}

static inline struct xhci_slot_state *xhci_ss(struct xhci_hc *hc, uint8_t slot_id) {
	if (!hc || slot_id == 0) {
		return NULL;
	}
	return &hc->slots[slot_id];
}

static void xhci_handle_unrelated_transfer_event(struct xhci_hc *hc, struct trb *e) {
	if (!hc || !e) {
		return;
	}

	switch (trb_get_type(e)) {
		case TRB_TRANSFER_EVENT: {
			/* Route INT-IN completions to the correct slot (or legacy dev if used). */
			uint8_t epid = trb_get_epid(e);
			if (epid == EPID_EP1_IN) {
				xhci_deliver_ep1in(hc, e, false);
			}
			break;
		}

		case TRB_CMD_COMPLETION:
		case TRB_PORT_STATUS:
		case TRB_HOST_CONTROLLER_EVENT:
		default:
			break;
	}
}

/* Decode a Transfer Event TRB -> slot_id, endpoint_id, cc, bytes (EVTL). */
static inline void xhci_decode_xfer_evt(const struct trb *e, uint8_t *slot_id, uint8_t *ep_id, uint8_t *cc, uint32_t *bytes) {
	/* Dword3: [31:24]=CC, [23:17]=rsvd, [16:8]=EPID, [7:0]=SlotID */
	uint32_t ctrl = e->ctrl;
	uint32_t sts = e->sts;
	*cc = (uint8_t) ((sts >> 24) & 0xFF);
	*ep_id = (uint8_t) ((ctrl >> 16) & 0x1F);
	*slot_id = (uint8_t) (ctrl & 0xFF);
	*bytes = (sts & 0x00FFFFFFu); /* EVTL: bytes remaining */
}

/* ring_init with debug - set Toggle Cycle (TC) like libpayload */
static void ring_init(struct xhci_ring *r, size_t bytes, uint64_t phys, void *virt) {
	r->base = (struct trb *) virt;
	r->phys = phys;
	r->num_trbs = (uint32_t) (bytes / sizeof(struct trb));
	r->enqueue = 0;
	r->cycle = 1;
	memset(r->base, 0, bytes);
	/* terminal link TRB: point to start, set TC=1, set C=1 (producer PCS) */
	struct trb *link = &r->base[r->num_trbs - 1];
	link->lo = (uint32_t) (r->phys & 0xFFFFFFFFu);
	link->hi = (uint32_t) (r->phys >> 32);
	link->sts = 0;
	link->ctrl = TRB_SET_TYPE(TRB_LINK) | TRB_TOGGLE | TRB_CYCLE;
	//dprintf("xhci.dbg:   TRBs=%u link_trb@%u lo=%08x hi=%08x ctrl=%08x\n", r->num_trbs, r->num_trbs - 1, link->lo, link->hi, link->ctrl);
}

static inline struct trb *ring_push(struct xhci_ring *r) {
	struct trb *cur = &r->base[r->enqueue];
	r->enqueue++;
	if (r->enqueue == r->num_trbs) {
		struct trb *link = &r->base[r->num_trbs - 1];

		link->ctrl = (link->ctrl & ~TRB_CYCLE) | (r->cycle ? TRB_CYCLE : 0);
		r->enqueue = 0;
		if (link->ctrl & TRB_TOGGLE)
			r->cycle ^= 1u;
		cur = &r->base[r->enqueue++];
	}
	memset(cur, 0, sizeof(*cur));
	return cur;
}

static int xhci_cmd_submit_wait(struct xhci_hc *hc, struct trb *cmd_trb) {
	uint64_t cmd_phys = hc->cmd.phys + ((uintptr_t) cmd_trb - (uintptr_t) hc->cmd.base);
	//dprintf("xhci.dbg: cmd_submit dbell0, trb_virt=%lx trb_phys=%lx ctrl=%08x\n", (uintptr_t) cmd_trb, cmd_phys, cmd_trb->ctrl);
	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;
	uint32_t iman_old = mmio_read32(ir0 + IR_IMAN);
	mmio_write32(ir0 + IR_IMAN, (uint32_t) (iman_old & ~IR_IMAN_IE));
	uint64_t erdp_start = mmio_read64(ir0 + IR_ERDP) & ~0x7ull;
	mmio_write64(ir0 + IR_ERDP, erdp_start | (1ull << 3));

	mmio_write32(hc->db + 0, 0);
	uint64_t deadline = get_ticks() + 50;
	uint64_t erdp_cur = erdp_start;
	for (;;) {
		for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
			struct trb *e = &hc->evt.base[i];
			if ((e->lo | e->hi | e->sts | e->ctrl) == 0u) {
				continue;
			}

			uint32_t type = (e->ctrl >> 10) & 0x3Fu;
			uint64_t evt_ptr = ((uint64_t) e->hi << 32) | e->lo;
			if (type == TRB_CMD_COMPLETION) {
				uint32_t cc = (e->sts >> 24) & 0xFFu;
				//dprintf("xhci.dbg:   evt[%u] type=%u lo=%08x hi=%08x sts=%08x cc=%02x match=%s\n", i, type, e->lo, e->hi, e->sts, cc, (evt_ptr == cmd_phys ? "yes" : "no"));

				if (evt_ptr == cmd_phys) {
					/* consume and bump ERDP by one TRB (keep EHB while in poll) */
					xhci_handle_unrelated_transfer_event(hc, e);
					memset(e, 0, sizeof(*e));
					erdp_cur = (erdp_cur + 16) & ~0x7ull;
					mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));

					/* leave with EHB CLEARED (poll-only: do not restore IE) */
					mmio_write64(ir0 + IR_ERDP, erdp_cur);

					if (cc != 0x01u) {
						dprintf("xhci.dbg:   command failed, cc=%02x\n", cc);
						return 0;
					}
					return 1;
				}
			}

			xhci_handle_unrelated_transfer_event(hc, e);
			memset(e, 0, sizeof(*e));
			erdp_cur = (erdp_cur + 16) & ~0x7ull;
			mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));
		}

		if ((int64_t) (get_ticks() - deadline) > 0) {
			break;
		}
		__builtin_ia32_pause();
	}

	/* timeout: clear EHB (poll-only: do not restore IE) */
	mmio_write64(ir0 + IR_ERDP, erdp_cur);
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

	if (!pci_bar_is_mem64(bar0)) {
		dprintf("xhci: BAR0 not 64-bit MMIO\n");
		return 0;
	}

	uint64_t base = pci_mem_base64(bar0, bar1);

	pci_enable_memspace(dev);
	pci_bus_master(dev);

	hc->cap = (volatile uint8_t *) (uintptr_t) base;

	hc->cap_len = (uint8_t) mmio_read32(hc->cap + XHCI_CAPLENGTH);
	uint32_t hcs1 = mmio_read32(hc->cap + XHCI_HCSPARAMS1);
	uint32_t hcs2 = mmio_read32(hc->cap + XHCI_HCSPARAMS2);
	uint32_t hcc1 = mmio_read32(hc->cap + XHCI_HCCPARAMS1);

	hc->op = hc->cap + hc->cap_len;
	hc->db = hc->cap + (mmio_read32(hc->cap + XHCI_DBOFF) & ~0x3u);
	hc->rt = hc->cap + (mmio_read32(hc->cap + XHCI_RTSOFF) & ~0x1Fu);

	hc->max_slots = (uint8_t) (hcs1 & 0xFFu);
	hc->port_count = (uint8_t) ((hcs1 >> 24) & 0xFFu);
	hc->csz64 = (hcc1 & (1u << 2)) ? 1 : 0;

	uint32_t sp_lo = (hcs2 >> 27) & 0x1Fu;
	uint32_t sp_hi = (hcs2 >> 21) & 0x1Fu;
	uint32_t sp_count = (sp_lo << 5) | sp_hi;
	if (sp_count == 0) {
		sp_count = 1;
	}

	/*dprintf("xhci.dbg:   cap_len=%u hcs1=%08x hcs2=%08x hcc1=%08x\n", hc->cap_len, hcs1, hcs2, hcc1);
	dprintf("xhci.dbg:   op=%lx db=%lx rt=%lx max_slots=%u ports=%u csz64=%u scratchpads=%u\n", (uintptr_t) hc->op, (uintptr_t) hc->db, (uintptr_t) hc->rt, hc->max_slots, hc->port_count, hc->csz64, sp_count);*/

	/* DCBAA + scratchpads */
	hc->dcbaa = (uint64_t *) kmalloc_aligned(4096, 4096);
	memset(hc->dcbaa, 0, 4096);
	hc->dcbaa_phys = (uint64_t) (uintptr_t) hc->dcbaa;

	hc->scratch_index = (uint64_t *) kmalloc_aligned(4096, 4096);
	memset(hc->scratch_index, 0, 4096);
	hc->scratch_index_phys = (uint64_t) (uintptr_t) hc->scratch_index;
	for (uint32_t i = 0; i < sp_count; i++) {
		void *pad = kmalloc_aligned(4096, 4096);
		memset(pad, 0, 4096);
		hc->scratch_index[i] = (uint64_t) (uintptr_t) pad;
	}
	hc->dcbaa[0] = hc->scratch_index_phys;

	//dprintf("xhci: BAR0=0x%lx caplen=%u max_slots=%u ports=%u scratchpads=%u csz64=%u\n", base, hc->cap_len, hc->max_slots, hc->port_count, sp_count, hc->csz64);
	return 1;
}

static int xhci_reset_controller(struct xhci_hc *hc) {
	//dprintf("xhci.dbg: reset_controller enter\n");

	uint32_t cmd = mmio_read32(hc->op + XHCI_USBCMD);
	/*uint32_t sts0 = mmio_read32(hc->op + XHCI_USBSTS);
	dprintf("xhci.dbg:   USBCMD=%08x USBSTS=%08x\n", cmd, sts0);*/

	/* halt if running */
	if (cmd & USBCMD_RS) {
		mmio_write32(hc->op + XHCI_USBCMD, cmd & ~USBCMD_RS);
		uint64_t until = get_ticks() + 20;
		while (((mmio_read32(hc->op + XHCI_USBSTS) & USBSTS_HCH) == 0) && get_ticks() < until) {
			__builtin_ia32_pause();
		}
		//dprintf("xhci.dbg:   halted, USBSTS=%08x\n", mmio_read32(hc->op + XHCI_USBSTS));
	}

	/* HCRST */
	mmio_write32(hc->op + XHCI_USBCMD, USBCMD_HCRST);
	uint64_t until = get_ticks() + 100;
	while ((mmio_read32(hc->op + XHCI_USBCMD) & USBCMD_HCRST) && get_ticks() < until) {
		__builtin_ia32_pause();
	}
	uint32_t sts1 = mmio_read32(hc->op + XHCI_USBSTS);
	//dprintf("xhci.dbg:   post HCRST USBSTS=%08x\n", sts1);
	if (sts1 & USBSTS_CNR) {
		dprintf("xhci: controller not ready after reset\n");
		return 0;
	}

	/* clear pending status */
	mmio_write32(hc->op + XHCI_USBSTS, sts1);

	/* DCBAA */
	mmio_write64(hc->op + XHCI_DCBAP, hc->dcbaa_phys);
	//dprintf("xhci.dbg:   DCBAP=%lx\n", hc->dcbaa_phys);

	/* command ring */
	void *cr_virt = kmalloc_aligned(4096, 4096);
	uint64_t cr_phys = (uint64_t) (uintptr_t) cr_virt;
	ring_init(&hc->cmd, 4096, cr_phys, cr_virt);
	mmio_write64(hc->op + XHCI_CRCR, (cr_phys & ~0xFu) | 1u);
	//dprintf("xhci.dbg:   CRCR=%lx\n", ((cr_phys & ~0xFu) | 1u));

	/* event ring (linear buffer, not link ring) */
	void *er_virt = kmalloc_aligned(4096, 4096);
	uint64_t er_phys = er_virt;
	memset(er_virt, 0, 4096);
	hc->evt.base = (struct trb *) er_virt;
	hc->evt.phys = er_phys;
	hc->evt.num_trbs = (uint32_t) (4096 / sizeof(struct trb));
	hc->evt.enqueue = 0;
	hc->evt.cycle = 1;
	hc->evt.dequeue = 0; /* software consumer index for the event ring */
	hc->evt.ccs = 1; /* consumer cycle state starts at 1 per xHCI */

	hc->erst = (struct erst_entry *) kmalloc_aligned(64, 64);
	memset(hc->erst, 0, 64);
	hc->erst[0].ring_base = er_phys;
	hc->erst[0].size = 256;
	hc->erst_phys = (uint64_t) (uintptr_t) hc->erst;

	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;
	mmio_write32(ir0 + IR_ERSTSZ, 1);
	mmio_write64(ir0 + IR_ERSTBA, hc->erst_phys);
	mmio_write64(ir0 + IR_ERDP, er_phys | (1ull << 3));

	mmio_write32(ir0 + IR_IMOD, 0);
	/* CHANGED: leave IE=0 for polled mode */
	mmio_write32(ir0 + IR_IMAN, 0);

	//dprintf("xhci.dbg:   ERSTSZ=1 ERSTBA=%lx ERDP=%lx IMOD=64 IMAN=%08x\n", hc->erst_phys, (uint64_t) (er_phys | (1ull << 3)), mmio_read32(ir0 + IR_IMAN));

	/* CONFIG, RS (no USBCMD_INTE in polled mode) */
	mmio_write32(hc->op + XHCI_CONFIG, hc->max_slots ? hc->max_slots : 8);
	/* CHANGED: drop INTE */
	mmio_write32(hc->op + XHCI_USBCMD, USBCMD_RS);

	until = get_ticks() + 20;
	while ((mmio_read32(hc->op + XHCI_USBSTS) & USBSTS_HCH) && get_ticks() < until) {
		__builtin_ia32_pause();
	}

	/*uint32_t sts2 = mmio_read32(hc->op + XHCI_USBSTS);
	uint32_t cmd2 = mmio_read32(hc->op + XHCI_USBCMD);
	dprintf("xhci.dbg:   after start USBCMD=%08x USBSTS=%08x\n", cmd2, sts2);*/
	return 1;
}

static void ctx_program_ep0(struct ep_ctx *e, uint16_t mps, uint64_t tr_deq_phys) {
	e->dword0 = (3u << 1); /* CErr */
	e->dword1 = ((uint32_t) mps << 16) | (4u << 8); /* Type */
	e->deq = (tr_deq_phys | 1u); /* DCS */
	e->dword4 = 8u; /* TRB len */
}

static int xhci_ctrl_build_and_run(struct xhci_hc *hc, uint8_t slot_id, const uint8_t *setup8, void *data, uint16_t len, int dir_in) {
	if (!hc || !setup8 || slot_id == 0) return 0;

	struct xhci_slot_state *ss = xhci_ss(hc, slot_id);
	if (!ss) return 0;

	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;

	uint32_t iman_old = mmio_read32(ir0 + IR_IMAN);
	mmio_write32(ir0 + IR_IMAN, (uint32_t) (iman_old & ~IR_IMAN_IE));
	uint64_t erdp_cur = mmio_read64(ir0 + IR_ERDP) & ~0x7ull;
	mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));

	do {
		if (!ss->ep0_tr.base) {
			void *v = kmalloc_aligned(4096, 4096);
			if (!v) break;
			uint64_t p = (uint64_t) (uintptr_t) v;
			ring_init(&ss->ep0_tr, 4096, p, v);
		}

		if (len) {
			if (!ss->ctrl_dma) {
				ss->ctrl_dma_sz = 65535;
				ss->ctrl_dma = kmalloc_aligned(ss->ctrl_dma_sz, 64);
				if (!ss->ctrl_dma) break;
				ss->ctrl_dma_phys = ss->ctrl_dma;
			}
			if (!dir_in) {
				memcpy(ss->ctrl_dma, data, len);
			}
		}

		struct trb *t_setup = ring_push(&ss->ep0_tr);
		t_setup->lo = *(const uint32_t *) &setup8[0];
		t_setup->hi = *(const uint32_t *) &setup8[4];
		uint32_t trt = len ? (dir_in ? 2 : 3) : 0;
		t_setup->sts = (trt << 16) | 8;
		t_setup->ctrl = TRB_SET_TYPE(TRB_SETUP_STAGE) | TRB_IDT | TRB_IOC | (ss->ep0_tr.cycle ? TRB_CYCLE : 0);

		if (len) {
			uint64_t cur = ss->ctrl_dma ? ss->ctrl_dma_phys : (uint64_t) (uintptr_t) data;
			uint32_t remain = len;
			while (remain) {
				uint64_t next_boundary = (cur + 0x10000ull) & ~0xFFFFull;
				uint32_t chunk = (remain < (uint32_t) (next_boundary - cur)) ? remain : (uint32_t) (next_boundary - cur);

				struct trb *t = ring_push(&ss->ep0_tr);
				t->lo = (uint32_t) (cur & 0xFFFFFFFFu);
				t->hi = (uint32_t) (cur >> 32);
				t->sts = chunk;
				t->ctrl = TRB_SET_TYPE(TRB_DATA_STAGE) | (dir_in ? TRB_DIR : 0) | TRB_CH | (ss->ep0_tr.cycle ? TRB_CYCLE : 0);

				cur += chunk;
				remain -= chunk;
			}

			struct trb *t_ev = ring_push(&ss->ep0_tr);
			t_ev->lo = (uint32_t) (uintptr_t) t_ev;
			t_ev->hi = 0;
			t_ev->sts = 0;
			t_ev->ctrl = TRB_SET_TYPE(TRB_EVENT_DATA) | TRB_IOC | (ss->ep0_tr.cycle ? TRB_CYCLE : 0);
		}

		struct trb *t_status = ring_push(&ss->ep0_tr);
		t_status->lo = 0;
		t_status->hi = 0;
		t_status->sts = 0;
		{
			const int status_in = (len == 0) ? 1 : (!dir_in);
			t_status->ctrl = TRB_SET_TYPE(TRB_STATUS_STAGE) | (status_in ? TRB_DIR : 0) | TRB_IOC | (ss->ep0_tr.cycle ? TRB_CYCLE : 0);
		}

		mmio_write32(hc->db + 4u * slot_id, EPID_CTRL);

		const uint32_t target_events = 2u + (len ? 1u : 0u);
		uint32_t got_events = 0u;
		uint64_t deadline = get_ticks() + 100;

		for (;;) {
			int progressed = 0;
			for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
				struct trb *e = &hc->evt.base[i];
				if ((e->lo | e->hi | e->sts | e->ctrl) == 0u) continue;

				uint32_t type = (e->ctrl >> 10) & 0x3Fu;

				if (type == TRB_TRANSFER_EVENT) {
					if (len && dir_in && ss->ctrl_dma) {
						memcpy(data, ss->ctrl_dma, len);
					}

					xhci_handle_unrelated_transfer_event(hc, e);
					memset(e, 0, sizeof(*e));
					erdp_cur = (erdp_cur + 16) & ~0x7ull;
					mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));

					got_events++;
					progressed = 1;
					if (got_events >= target_events) {
						mmio_write64(ir0 + IR_ERDP, erdp_cur);
						return 1;
					}
					continue;
				}

				xhci_handle_unrelated_transfer_event(hc, e);
				memset(e, 0, sizeof(*e));
				erdp_cur = (erdp_cur + 16) & ~0x7ull;
				mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));
				progressed = 1;
			}

			if (!progressed) {
				if ((int64_t) (get_ticks() - deadline) > 0) break;
				__builtin_ia32_pause();
			}
		}
		break;
	} while (0);

	mmio_write64(ir0 + IR_ERDP, erdp_cur);
	dprintf("xhci.dbg:   ctrl xfer timeout/fail (slot=%u)\n", slot_id);
	return 0;
}

/* simple wrapper for external users (hid etc.) */
bool xhci_ctrl_xfer(const struct usb_dev *ud, const uint8_t *setup, void *data, uint16_t len, int data_dir_in) {
	if (!ud || !ud->hc || !setup) {
		return false;
	}
	struct xhci_hc *hc = (struct xhci_hc *) ud->hc;
	return xhci_ctrl_build_and_run(hc, ud->slot_id, setup, data, len, data_dir_in);
}

static void xhci_deliver_ep1in(struct xhci_hc *hc, const struct trb *e, bool deliver_to_driver) {
	if (!hc || !e) return;

	uint8_t slot_id = trb_get_slotid(e);
	uint8_t epid = trb_get_epid(e);
	if (epid != EPID_EP1_IN) return;

	struct xhci_slot_state *ss = xhci_ss(hc, slot_id);
	if (!ss) return;
	if (!ss->int_cb || !ss->int_buf || !ss->int_pkt_len) return;

	uint8_t cc = trb_get_cc(e);
	uint32_t rem = trb_get_bytes_remaining(e);
	uint32_t req = ss->int_pkt_len;

	if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET) {
		uint32_t xfer = (rem <= req) ? (req - rem) : req;

		if (deliver_to_driver && xfer > 0) {
			struct usb_dev ud = {0};
			ud.hc = hc;
			ud.slot_id = slot_id;
			ss->int_cb(&ud, ss->int_buf, (uint16_t) xfer);
		}

		struct trb *n = ring_push(&ss->int_in_tr);
		n->lo = (uint32_t) (ss->int_buf_phys & 0xFFFFFFFFu);
		n->hi = (uint32_t) (ss->int_buf_phys >> 32);
		n->sts = ss->int_pkt_len;
		n->ctrl = TRB_SET_TYPE(TRB_NORMAL) | TRB_IOC | (ss->int_in_tr.cycle ? TRB_CYCLE : 0);

		mmio_write32(hc->db + 4u * slot_id, EPID_EP1_IN);
	} else {
		dprintf("xhci.dbg: EP1-IN TE cc=%u rem=%u (slot=%u)\n", cc, rem, slot_id);
	}
}

static void xhci_isr(uint8_t isr, uint64_t error, uint64_t irq, void *opaque) {
	/* This is a stub, the xhci driver is driven by a deterministic poll */
}

bool xhci_arm_int_in(const struct usb_dev *ud, uint16_t pkt_len, xhci_int_in_cb cb) {
	//dprintf("xhci.dbg: ARM INT-IN enter ud=%p hc=%p slot_id=%u pkt_len=%u cb=%p\n", ud, ud ? ud->hc : 0, ud ? ud->slot_id : 0, pkt_len, cb);

	if (!ud || !ud->hc || !pkt_len || !cb) {
		dprintf("xhci.dbg: ARM INT-IN early-exit (arg check) ud=%p hc=%p pkt_len=%u cb=%p\n", ud, ud ? ud->hc : 0, pkt_len, cb);
		return false;
	}
	struct xhci_hc *hc = (struct xhci_hc *) ud->hc;
	struct xhci_slot_state *ss = xhci_ss(hc, ud->slot_id);
	if (!ss) {
		return false;
	}

	/* Use discovered INT-IN from config (fallbacks retained). */
	uint8_t ep_num = ss->int_ep_num ? ss->int_ep_num : 1;
	uint16_t ep_mps = ss->int_ep_mps ? ss->int_ep_mps : 8;
	uint8_t ep_bInterval = ss->int_ep_interval ? ss->int_ep_interval : 1;

	/* Only EP1 IN is supported in this driver (EPID==3). */
	if (ep_num != 1) {
		uint8_t epid = (uint8_t) (2u * ep_num + 1u);
		dprintf("xhci.dbg: WARNING: device INT-IN at EP%u (EPID=%u), driver only supports EP1 IN; not arming.\n", ep_num, epid);
		return false;
	}

	/* Ensure TR for EP1 IN */
	if (!ss->int_in_tr.base) {
		void *v = kmalloc_aligned(4096, 4096);
		if (!v) {
			dprintf("xhci.dbg: INT-IN TR alloc fail\n");
			return false;
		}
		uint64_t p = (uint64_t) (uintptr_t) v;
		//dprintf("xhci.dbg: INT-IN TR alloc v=%p phys=%lx\n", v, p);
		ring_init(&ss->int_in_tr, 4096, p, v);
		//dprintf("xhci.dbg: INT-IN TR init base=%p phys=%lx cycle=%u\n", ss->int_in_tr.base, ss->int_in_tr.phys, ss->int_in_tr.cycle);
	} else {
		dprintf("xhci.dbg: INT-IN TR already init base=%p phys=%lx cycle=%u\n", ss->int_in_tr.base, ss->int_in_tr.phys, ss->int_in_tr.cycle);
	}

	/* Ensure report buffer */
	if (!ss->int_buf) {
		ss->int_buf = (uint8_t *) kmalloc_aligned(pkt_len, 64);
		if (!ss->int_buf) {
			dprintf("xhci.dbg: INT-IN buf alloc fail (%u)\n", pkt_len);
			return false;
		}
		ss->int_buf_phys = (uint64_t) (uintptr_t) ss->int_buf;
		//dprintf("xhci.dbg: INT-IN buf alloc v=%p phys=%lx len=%u\n", ss->int_buf, ss->int_buf_phys, (unsigned) pkt_len);
	} else {
		dprintf("xhci.dbg: INT-IN buf reuse v=%p phys=%lx len=%u\n", ss->int_buf, ss->int_buf_phys, (unsigned) pkt_len);
	}
	ss->int_cb = cb;
	ss->int_pkt_len = pkt_len;

	/* Ensure we have an input context for this slot */
	if (!ss->ic) {
		ss->ic = (struct input_ctx *) kmalloc_aligned(4096, 64);
		if (!ss->ic) {
			dprintf("xhci.dbg:   input ctx alloc failed (slot=%u)\n", ud->slot_id);
			return false;
		}
		ss->ic_phys = (uint64_t) (uintptr_t) ss->ic;
		memset(ss->ic, 0, 4096);
	}

	/* Copy current Device Slot Context -> Input Slot Context; set CE=3. */
	uint64_t dphys = hc->dcbaa[ud->slot_id];
	volatile uint32_t *dcs = (volatile uint32_t *) (uintptr_t) dphys;
	volatile uint32_t *isc = (volatile uint32_t *) &ss->ic->slot;
	for (int k = 0; k < 8; k++) isc[k] = dcs[k];
	isc[0] = (isc[0] & ~(0x1Fu << 27)) | (3u << 27);

	/* Interval encoding (same logic you already had) */
	uint8_t speed_code = (uint8_t) (ss->ic->slot.dword2 & 0xF);
	uint32_t intval = 1;
	if (speed_code >= 3) {
		uint8_t bI = ep_bInterval ? ep_bInterval : 1;
		intval = (bI > 0) ? (uint32_t) (bI - 1) : 0u;
	} else {
		intval = ep_bInterval ? ep_bInterval : 1;
		if (intval < 3) intval = 3;
		else if (intval > 11) intval = 11;
	}
	if (intval > 15) intval = 15;

	memset(&ss->ic->ep1_in, 0, sizeof(ss->ic->ep1_in));
	ss->ic->ep1_in.dword0 = (3u << 1);
	ss->ic->ep1_in.dword1 = ((uint32_t) ep_mps << 16) | ((intval & 0xFFu) << 8) | (7u << 3);
	ss->ic->ep1_in.deq = (ss->int_in_tr.phys | 1u);
	{
		uint32_t avg_trb_len = 1024;
		uint32_t max_esit = ep_mps;
		ss->ic->ep1_in.dword4 = (avg_trb_len << 16) | max_esit;
	}

	ss->ic->drop_flags = 0;
	ss->ic->add_flags = 0x00000009;

	struct trb *ce = xhci_cmd_begin(hc);
	ce->lo = (uint32_t) (ss->ic_phys & 0xFFFFFFFFu);
	ce->hi = (uint32_t) (ss->ic_phys >> 32);
	ce->ctrl |= TRB_SET_TYPE(TRB_CONFIGURE_EP) | ((uint32_t) ud->slot_id << 24);
	//dprintf("xhci.dbg: ring_push for CONFIGURE_EP trb=%08x ic_phys_lo=%08x ic_phys_hi=%08x ctrl=%08x slot=%u\n", (uint32_t) (uintptr_t) ce, (uint32_t) (ss->ic_phys & 0xFFFFFFFFu), (uint32_t) (ss->ic_phys >> 32), ce->ctrl, ud->slot_id);

	if (!xhci_cmd_submit_wait(hc, ce)) {
		dprintf("xhci.dbg: CONFIGURE_EP timeout/fail\n");
		return false;
	}
	//dprintf("xhci.dbg: CONFIGURE_EP success\n");

	for (int k = 0; k < 2; ++k) {
		struct trb *n = ring_push(&ss->int_in_tr);
		n->lo = (uint32_t) (ss->int_buf_phys & 0xFFFFFFFFu);
		n->hi = (uint32_t) (ss->int_buf_phys >> 32);
		n->sts = ss->int_pkt_len;
		n->ctrl = TRB_SET_TYPE(TRB_NORMAL) | TRB_IOC | (ss->int_in_tr.cycle ? TRB_CYCLE : 0);
	}
	mmio_write32(hc->db + 4u * ud->slot_id, EPID_EP1_IN);
	//dprintf("xhci.dbg:   doorbell EP1 IN (slot=%u) tr.phys=%lx buf.phys=%lx (prepost=2)\n", ud->slot_id, ss->int_in_tr.phys, ss->int_buf_phys);

	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;

	mmio_write32(ir0 + IR_IMOD, 0);

	uint32_t sts = mmio_read32(hc->op + XHCI_USBSTS);
	if (sts & USBSTS_EINT) mmio_write32(hc->op + XHCI_USBSTS, USBSTS_EINT);

	uint32_t iman = mmio_read32(ir0 + IR_IMAN);
	if (iman & IR_IMAN_IP) mmio_write32(ir0 + IR_IMAN, IR_IMAN_IP);

	uint64_t erdp = mmio_read64(ir0 + IR_ERDP) & ~0x7ull;
	mmio_write64(ir0 + IR_ERDP, erdp);

	return true;
}

bool xhci_probe_and_init(uint8_t bus, uint8_t dev, uint8_t func) {
	pci_dev_t p = {0};
	p.bus_num = bus;
	p.device_num = dev;
	p.function_num = func;

	g_xhci = kmalloc_aligned(sizeof(*g_xhci), 4096);
	memset(g_xhci, 0, sizeof(*g_xhci));

	if (!xhci_hw_enable_map(g_xhci, p)) {
		kfree_null(&g_xhci);
		return false;
	}
	if (!xhci_reset_controller(g_xhci)) {
		kfree_null(&g_xhci);
		return false;
	}

	xhci_disable_interrupts(g_xhci);

	uint32_t irq_line = pci_read(p, PCI_INTERRUPT_LINE) & 0xFFu;
	uint32_t irq_pin = pci_read(p, PCI_INTERRUPT_PIN) & 0xFFu;
	g_xhci->irq_line = (uint8_t) irq_line;
	g_xhci->irq_pin = (uint8_t) irq_pin;

	pci_setup_interrupt("xhci", p, logical_cpu_id(), xhci_isr, g_xhci);
	dprintf("xhci: using legacy INTx IRQ=%u (PIN#%c)\n", irq_line, 'A' + (int) irq_pin - 1);

	/* Enumerate all connected ports; publish each device */
	uint32_t published = 0;

	for (uint8_t pidx = 0; pidx < g_xhci->port_count; pidx++) {
		volatile uint8_t *pr = g_xhci->op + XHCI_PORTREG_BASE + pidx * XHCI_PORT_STRIDE;
		uint32_t sc = mmio_read32(pr + PORTSC);
		if ((sc & PORTSC_CCS) == 0) {
			continue;
		}

		/* Reset the port (W1C change bits + PR) */
		mmio_write32(pr + PORTSC, (sc & PORTSC_RW1C_MASK) | PORTSC_PR);

		/* ~100ms settle */
		uint64_t end_wait = get_ticks() + 100;
		while (get_ticks() < end_wait) {
			__builtin_ia32_pause();
		}

		/* Enable a slot. In QEMU the assigned Slot IDs are 1,2,3,... */
		struct trb *es = xhci_cmd_begin(g_xhci);
		es->ctrl |= TRB_SET_TYPE(TRB_ENABLE_SLOT);
		if (!xhci_cmd_submit_wait(g_xhci, es)) {
			dprintf("xhci.dbg: ENABLE_SLOT failed on port %u\n", pidx + 1);
			continue;
		}
		uint8_t slot_id = (uint8_t) (published + 1);
		if (slot_id == 0) {
			dprintf("xhci.dbg: slot id out of range\n");
			break;
		}

		struct xhci_slot_state *ss = xhci_ss(g_xhci, slot_id);
		if (!ss) {
			break;
		}
		memset(ss, 0, sizeof(*ss));
		ss->in_use = true;
		ss->slot_id = slot_id;
		ss->port = pidx + 1;

		/* Allocate and clear Input Context */
		if (!ss->ic) {
			ss->ic = (struct input_ctx *) kmalloc_aligned(4096, 64);
			if (!ss->ic) {
				dprintf("xhci.dbg:   input ctx alloc failed (port=%u)\n", pidx + 1);
				continue;
			}
			ss->ic_phys = (uint64_t) (uintptr_t) ss->ic;
		}
		memset(ss->ic, 0, 4096);

		/* Device Context page and DCBAA[slot] */
		void *devctx = kmalloc_aligned(4096, 64);
		if (!devctx) {
			dprintf("xhci.dbg:   device ctx alloc failed\n");
			continue;
		}
		memset(devctx, 0, 4096);
		g_xhci->dcbaa[slot_id] = (uint64_t) (uintptr_t) devctx;

		/* EP0 transfer ring */
		if (!ss->ep0_tr.base) {
			void *tr0 = kmalloc_aligned(4096, 4096);
			if (!tr0) {
				dprintf("xhci.dbg:   ep0 ring alloc failed\n");
				continue;
			}
			uint64_t tr0p = (uint64_t) (uintptr_t) tr0;
			ring_init(&ss->ep0_tr, 4096, tr0p, tr0);
		}

		/* Fill Input Context slot + ep0 */
		volatile uint8_t *pr_att = g_xhci->op + XHCI_PORTREG_BASE + pidx * XHCI_PORT_STRIDE;
		uint32_t psc = mmio_read32(pr_att + PORTSC);
		uint32_t speed = (psc >> 10) & 0xFu;
		ss->speed = (int) speed;

		ss->ic->add_flags = 0x00000003u;        /* A0|A1 */
		ss->ic->slot.dword0 = (1u << 27);       /* CE=1 (slot + ep0) */
		ss->ic->slot.dword1 = ((uint32_t) (pidx + 1) << 16);
		ss->ic->slot.dword2 = speed;
		ctx_program_ep0(&ss->ic->ep0, 8u, ss->ep0_tr.phys);

		/* Address Device with BSR=1 */
		{
			struct trb *ad = xhci_cmd_begin(g_xhci);
			ad->lo = (uint32_t) (ss->ic_phys & 0xFFFFFFFFu);
			ad->hi = (uint32_t) (ss->ic_phys >> 32);
			ad->ctrl |= TRB_SET_TYPE(TRB_ADDRESS_DEVICE) | ((uint32_t) slot_id << 24) | (1u << 9);
			if (!xhci_cmd_submit_wait(g_xhci, ad)) {
				dprintf("xhci.dbg:   ADDRESS_DEVICE (BSR=1) fail\n");
				continue;
			}
		}

		/* DEV DESC 8 bytes (for MPS0) */
		uint8_t __attribute__((aligned(64))) setup_dev8[8] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 8, 0x00};
		uint8_t __attribute__((aligned(64))) dev_desc[256];
		memset(dev_desc, 0, sizeof(dev_desc));
		if (!xhci_ctrl_build_and_run(g_xhci, slot_id, setup_dev8, dev_desc, 8, 1)) {
			dprintf("xhci.dbg:   dev transfer timeout (GET_DESCRIPTOR 8)\n");
			continue;
		}

		uint16_t mps = dev_desc[7];
		if (mps != 8) {
			ss->ic->add_flags = 0x00000002u;
			ctx_program_ep0(&ss->ic->ep0, mps, ss->ep0_tr.phys);

			struct trb *ev = xhci_cmd_begin(g_xhci);
			ev->lo = (uint32_t) (ss->ic_phys & 0xFFFFFFFFu);
			ev->hi = (uint32_t) (ss->ic_phys >> 32);
			ev->ctrl |= TRB_SET_TYPE(TRB_EVAL_CONTEXT) | ((uint32_t) slot_id << 24);
			if (!xhci_cmd_submit_wait(g_xhci, ev)) {
				dprintf("xhci.dbg:   EVAL_CONTEXT fail\n");
				continue;
			}
		}

		ss->ic->add_flags = 0x00000003u;
		ss->ic->drop_flags = 0;
		ctx_program_ep0(&ss->ic->ep0, mps, ss->ep0_tr.phys);

		/* Assign address (BSR=0) */
		{
			struct trb *ad2 = xhci_cmd_begin(g_xhci);
			ad2->lo = (uint32_t) (ss->ic_phys & 0xFFFFFFFFu);
			ad2->hi = (uint32_t) (ss->ic_phys >> 32);
			ad2->ctrl |= TRB_SET_TYPE(TRB_ADDRESS_DEVICE) | ((uint32_t) slot_id << 24);
			if (!xhci_cmd_submit_wait(g_xhci, ad2)) {
				dprintf("xhci.dbg:   ADDRESS_DEVICE (assign) fail\n");
				continue;
			}
		}

		/* >>>>>> NEW: get full 18-byte device descriptor so VID/PID are valid <<<<<< */
		{
			uint8_t __attribute__((aligned(64))) setup_dev_full[8] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 18, 0x00};
			if (!xhci_ctrl_build_and_run(g_xhci, slot_id, setup_dev_full, dev_desc, 18, 1)) {
				dprintf("xhci.dbg:   dev transfer timeout (GET_DESCRIPTOR 18)\n");
				continue;
			}
		}

		/* Read config descriptor (9, then full) */
		uint8_t cfg_buf[512] __attribute__((aligned(64)));
		uint8_t  __attribute__((aligned(64))) setup_cfg9[8] = {0x80, 0x06, 0x00, 0x02, 0x00, 0x00, 9, 0x00};
		if (!xhci_ctrl_build_and_run(g_xhci, slot_id, setup_cfg9, cfg_buf, 9, 1)) {
			continue;
		}
		uint16_t total_len = (uint16_t) cfg_buf[2] | ((uint16_t) cfg_buf[3] << 8);
		if (total_len > sizeof(cfg_buf)) total_len = sizeof(cfg_buf);
		uint8_t setup_cfg_full[8] = {
			0x80, 0x06, 0x00, 0x02, 0x00, 0x00,
			(uint8_t) (total_len & 0xFFu), (uint8_t) (total_len >> 8)
		};
		if (!xhci_ctrl_build_and_run(g_xhci, slot_id, setup_cfg_full, cfg_buf, total_len, 1)) {
			continue;
		}

		/* Parse first interface */
		uint8_t dev_class = 0, dev_sub = 0, dev_proto = 0, iface_num = 0;
		for (uint16_t off = 9; off + 2 <= total_len;) {
			uint8_t len = cfg_buf[off];
			uint8_t typ = cfg_buf[off + 1];
			if (len < 2) break;
			if (typ == 0x04 && len >= 9) {
				dev_class = cfg_buf[off + 5];
				dev_sub = cfg_buf[off + 6];
				dev_proto = cfg_buf[off + 7];
				iface_num = cfg_buf[off + 2];
				break;
			}
			off = (uint16_t) (off + len);
		}

		/* Discover first Interrupt IN endpoint on interface 0 alt 0. */
		uint8_t int_ep_num = 1;
		uint16_t int_ep_mps = 8;
		uint8_t int_ep_interval = 1;
		uint8_t cur_if = 0xFF, cur_alt = 0xFF;
		for (uint16_t off = 9; off + 2 <= total_len;) {
			uint8_t len = cfg_buf[off];
			uint8_t typ = cfg_buf[off + 1];
			if (len < 2) break;

			if (typ == 0x04 && len >= 9) {
				cur_if = cfg_buf[off + 2];
				cur_alt = cfg_buf[off + 3];
			} else if (typ == 0x05 && len >= 7) {
				uint8_t addr = cfg_buf[off + 2];
				uint8_t attr = cfg_buf[off + 3];
				uint16_t wMax = (uint16_t) cfg_buf[off + 4] | ((uint16_t) cfg_buf[off + 5] << 8);
				uint8_t intr = cfg_buf[off + 6];

				if (cur_if == 0 && cur_alt == 0 &&
				    (attr & 0x03) == 3 /* Interrupt */ &&
				    (addr & 0x80)) {
					int_ep_num = (uint8_t) (addr & 0x0F);
					int_ep_mps = wMax;
					int_ep_interval = intr ? intr : 1;
					break;
				}
			}
			off = (uint16_t) (off + len);
		}

		ss->int_ep_num = int_ep_num;
		ss->int_ep_mps = int_ep_mps;
		ss->int_ep_interval = int_ep_interval;

		struct usb_dev ud;
		memset(&ud, 0, sizeof(ud));
		ud.hc = g_xhci;
		ud.slot_id = slot_id;
		ud.address = (uint8_t) (published + 1);
		ud.vid = (uint16_t) dev_desc[8] | ((uint16_t) dev_desc[9] << 8);
		ud.pid = (uint16_t) dev_desc[10] | ((uint16_t) dev_desc[11] << 8);
		ud.iface_num = iface_num;
		ud.dev_class = dev_class;
		ud.dev_subclass = dev_sub;
		ud.dev_proto = dev_proto;
		ud.ep0.epid = EPID_CTRL;
		ud.ep0.mps = mps;
		ud.ep0.type = 0;
		ud.ep0.dir_in = 0;

		usb_core_device_added(&ud);
		published++;
	}

	dprintf("USB xHCI: controller ready, published %u device(s)\n", published ? published : 0);
	return true;
}

static inline void usb_fill_setup(uint8_t *setup, uint8_t bm, uint8_t bReq, uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
	setup[0] = bm;
	setup[1] = bReq;
	setup[2] = (uint8_t)(wValue & 0xFF);
	setup[3] = (uint8_t)(wValue >> 8);
	setup[4] = (uint8_t)(wIndex & 0xFF);
	setup[5] = (uint8_t)(wIndex >> 8);
	setup[6] = (uint8_t)(wLength & 0xFF);
	setup[7] = (uint8_t)(wLength >> 8);
}

/* ---- generic control sugar ---- */
bool usb_ctrl_nodata(const struct usb_dev *ud, uint8_t bm, uint8_t bReq, uint16_t wValue, uint16_t wIndex) {
	uint8_t setup[8] __attribute__((aligned(64)));
	usb_fill_setup(setup, bm, bReq, wValue, wIndex, 0);
	return xhci_ctrl_xfer(ud, setup, NULL, 0, 0);
}

bool usb_ctrl_get(const struct usb_dev *ud, uint8_t bm, uint8_t bReq, uint16_t wValue, uint16_t wIndex, void *buf, uint16_t len) {
	uint8_t setup[8] __attribute__((aligned(64)));
	usb_fill_setup(setup, bm, bReq, wValue, wIndex, len);
	return xhci_ctrl_xfer(ud, setup, buf, len, 1);
}

bool usb_ctrl_set(const struct usb_dev *ud, uint8_t bm, uint8_t bReq, uint16_t wValue, uint16_t wIndex, const void *buf, uint16_t len) {
	uint8_t setup[8] __attribute__((aligned(64)));
	usb_fill_setup(setup, bm, bReq, wValue, wIndex, len);
	return xhci_ctrl_xfer(ud, setup, (void *)buf, len, 0);
}

static void xhci_disable_interrupts(struct xhci_hc *hc) {
	if (!hc || !hc->rt || !hc->op) {
		return;
	}
	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;

	/* Mask interrupter 0 */
	uint32_t iman = mmio_read32(ir0 + IR_IMAN);
	mmio_write32(ir0 + IR_IMAN, (uint32_t) (iman & ~IR_IMAN_IE));

	/* Clear pending IP (W1C) */
	iman = mmio_read32(ir0 + IR_IMAN);
	if (iman & IR_IMAN_IP) {
		mmio_write32(ir0 + IR_IMAN, (uint32_t) (iman | IR_IMAN_IP));
	}

	/* Clear global INTE so HC won’t signal legacy/MSI at all */
	uint32_t usbcmd = mmio_read32(hc->op + XHCI_USBCMD);
	mmio_write32(hc->op + XHCI_USBCMD, (uint32_t) (usbcmd & ~USBCMD_INTE));
}

void xhci_poll(void) {
	struct xhci_hc *hc = g_xhci;
	if (!hc || !hc->cap) return;

	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;

	/* Drain from current SW dequeue with EHB=1 while we read events. */
	uint32_t n = hc->evt.num_trbs;
	uint32_t idx = hc->evt.dequeue;
	uint8_t ccs = (uint8_t) (hc->evt.ccs ? 1u : 0u);

	uint64_t erdp_claim = hc->evt.phys + ((uint64_t) idx << 4);
	uint64_t erdp_save = mmio_read64(ir0 + IR_ERDP) & ~0x7ull;
	mmio_write64(ir0 + IR_ERDP, erdp_claim | (1ull << 3));

	int consumed = 0;
	int resynced = 0;

	for (;;) {
		struct trb *e = &hc->evt.base[idx];

		/* Stop when producer cycle != our CCS: no new events (or out of phase). */
		if (((e->ctrl & TRB_CYCLE) ? 1u : 0u) != ccs) {
			/* One-shot resync: if an interrupt-pending bit says there SHOULD be events,
			   flip CCS once to match producer and try again. */
			if (!resynced) {
				uint32_t iman = mmio_read32(ir0 + IR_IMAN);
				uint32_t usbsts = mmio_read32(hc->op + XHCI_USBSTS);
				if ((iman & IR_IMAN_IP) || (usbsts & USBSTS_EINT)) {
					ccs ^= 1u;
					resynced = 1;
					continue;
				}
			}
			break;
		}

		uint8_t type = trb_get_type(e);

		if (type == TRB_TRANSFER_EVENT) {
			uint8_t epid = trb_get_epid(e);
			/* Only deliver to HID for EP1-IN completions */
			if (epid == EPID_EP1_IN) {
				xhci_deliver_ep1in(hc, e, true);
			} else {
				xhci_handle_unrelated_transfer_event(hc, e);
			}
		} else {
			xhci_handle_unrelated_transfer_event(hc, e);
		}

		/* Advance SW consumer (do not zero producer-owned TRB). */
		idx++;
		consumed++;
		if (idx == n) {
			idx = 0;
			ccs ^= 1u; /* flip Consumer Cycle State on wrap */
		}
	}

	/* Commit new dequeue (clears EHB at the committed position). */
	if (consumed) {
		hc->evt.dequeue = idx;
		hc->evt.ccs = ccs;

		uint64_t erdp_next = hc->evt.phys + ((uint64_t) idx << 4);
		mmio_write64(ir0 + IR_ERDP, erdp_next);
	} else {
		/* No events: restore prior ERDP and clear EHB. */
		mmio_write64(ir0 + IR_ERDP, erdp_save);
	}

	/* Best-effort W1C for IMAN.IP (safe even with IE/INTE disabled). */
	{
		uint32_t iman = mmio_read32(ir0 + IR_IMAN);
		if (iman & IR_IMAN_IP)
			mmio_write32(ir0 + IR_IMAN, (uint32_t) (iman | IR_IMAN_IP));
	}
}


/* find first USB controller by class code 0x0C03 and ensure xHCI (prog-if 0x30) */
void init_usb_xhci(void) {
	if (!running_under_qemu()) {
		return;
	}
	/* match by class only, like AHCI init uses 0x0106 */
	pci_dev_t usb = pci_get_device(0, 0, 0x0C03);
	if (!usb.bits) {
		dprintf("USB: no USB controllers found (class 0x0C03)\n");
		return;
	}

	/* check programming interface is xHCI (0x30) */
	uint32_t prog_if = pci_read(usb, PCI_PROG_IF);
	if (prog_if != 0x30) {
		dprintf("USB: controller at %u:%u.%u is not xHCI (prog-if %02x)\n", usb.bus_num, usb.device_num, usb.function_num, prog_if);
		return;
	}

	dprintf("USB: xHCI candidate at %u:%u.%u\n", usb.bus_num, usb.device_num, usb.function_num);

	/* one-and-done bring-up; xhci code handles BARs, interrupts, and enumeration */
	if (!xhci_probe_and_init(usb.bus_num, usb.device_num, usb.function_num)) {
		dprintf("USB: xHCI init failed at %u:%u.%u\n", usb.bus_num, usb.device_num, usb.function_num);
		return;
	}

	proc_register_idle(xhci_poll, IDLE_BACKGROUND, 1);

	dprintf("USB: xHCI initialised at %u:%u.%u\n", usb.bus_num, usb.device_num, usb.function_num);
}

/* ---- bulk helpers ---- */

static inline uint8_t xhci_epid_from(uint8_t ep_num, int dir_in) {
	/* EPID: 1=EP0, then pairs 2/3=EP1 OUT/IN, 4/5=EP2 OUT/IN, ... */
	return (uint8_t)(ep_num == 0 ? 1 : (uint8_t)(2u * ep_num + (dir_in ? 1u : 0u)));
}

static void xhci_program_bulk_ep_ctx(struct ep_ctx *e, uint16_t mps, uint64_t tr_deq_phys, int dir_in) {
	uint32_t ep_type = dir_in ? 6u : 2u; /* xHCI EPType: 2=Bulk OUT, 6=Bulk IN (bits [5:3]) */
	memset(e, 0, sizeof(*e));
	e->dword0 = (3u << 1); /* CErr = 3 */
	e->dword1 = ((uint32_t)mps << 16) | (ep_type << 3); /* Interval=0 for bulk */
	e->deq = (tr_deq_phys | 1u); /* DCS=1 */
	e->dword4 = (1024u << 16) | mps; /* AvgTRBLen=1024, Max ESIT Payload ~ MPS */
}

/* Configure (add) a single bulk endpoint using Configure Endpoint */
static bool xhci_configure_single_ep(struct xhci_hc *hc, uint8_t slot_id, uint8_t epid) {
	/* Copy current Slot Context from Device Context into Input Context, set CE=max(epid, current) */
	uint64_t dphys = hc->dcbaa[slot_id];
	volatile uint32_t *dcs = (volatile uint32_t *)(uintptr_t)dphys;
	volatile uint32_t *isc = (volatile uint32_t *)&xhci_ss(hc, slot_id)->ic->slot;
	for (int k = 0; k < 8; k++) isc[k] = dcs[k];
	uint32_t ce_val = (uint32_t)epid; /* highest context ID present */
	isc[0] = (isc[0] & ~(0x1Fu << 27)) | (ce_val << 27);

	/* A0 (slot) and A<epid> */
	struct xhci_slot_state *ss = xhci_ss(hc, slot_id);
	ss->ic->drop_flags = 0;
	ss->ic->add_flags = (1u << 0) | (1u << epid);

	/* Configure Endpoint command */
	struct trb *t = xhci_cmd_begin(hc);
	t->lo = (uint32_t)(ss->ic_phys & 0xFFFFFFFFu);
	t->hi = (uint32_t)(ss->ic_phys >> 32);
	t->ctrl |= TRB_SET_TYPE(TRB_CONFIGURE_EP) | ((uint32_t)slot_id << 24);
	if (!xhci_cmd_submit_wait(hc, t)) {
		dprintf("xhci.dbg: CONFIGURE_EP (EPID=%u) timeout/fail\n", epid);
		return false;
	}
	return true;
}

/* Public: open a bulk pipe (creates TR, programs EP ctx, issues CONFIGURE_EP) */
bool xhci_open_bulk_pipe(const struct usb_dev *ud, uint8_t ep_num, int dir_in, uint16_t mps) {
	if (!ud || !ud->hc || ep_num == 0) {
		dprintf("xhci: bulk open arg check fail ud=%p ep=%u\n", ud, ep_num);
		return false;
	}
	struct xhci_hc *hc = (struct xhci_hc *)ud->hc;
	struct xhci_slot_state *ss = xhci_ss(hc, ud->slot_id);
	if (!ss || !ss->ic) {
		dprintf("xhci: bulk open no slot/ic (slot=%u)\n", ud->slot_id);
		return false;
	}

	/* Ensure a TR for this direction */
	struct xhci_ring *r = dir_in ? &ss->bulk_in_tr : &ss->bulk_out_tr;
	if (!r->base) {
		void *v = kmalloc_aligned(4096, 4096);
		if (!v) {
			dprintf("xhci: bulk TR alloc fail\n");
			return false;
		}
		uint64_t p = (uint64_t)(uintptr_t)v;
		ring_init(r, 4096, p, v);
	}

	/* Program the appropriate ep_ctx in input context */
	uint8_t epid = xhci_epid_from(ep_num, dir_in);
	switch (epid) {
		case 4: xhci_program_bulk_ep_ctx(&ss->ic->ep2_out, mps, ss->bulk_out_tr.phys, 0); ss->bulk_out_mps = mps; break;
		case 5: xhci_program_bulk_ep_ctx(&ss->ic->ep2_in,  mps, ss->bulk_in_tr.phys,  1); ss->bulk_in_mps  = mps; break;
			/* For completeness, EP1 bulk case if a device uses EP1 bulk (rare when HID present) */
		case 2: xhci_program_bulk_ep_ctx(&ss->ic->ep1_out, mps, ss->bulk_out_tr.phys, 0); ss->bulk_out_mps = mps; break;
		case 3: xhci_program_bulk_ep_ctx(&ss->ic->ep1_in,  mps, ss->bulk_in_tr.phys,  1); ss->bulk_in_mps  = mps; break;
		default:
			dprintf("xhci: bulk EPID=%u not mapped in input_ctx\n", epid);
			return false;
	}

	/* Configure just this endpoint */
	if (!xhci_configure_single_ep(hc, ud->slot_id, epid)) {
		return false;
	}

	dprintf("xhci: bulk EP%u %s opened (EPID=%u MPS=%u)\n",
		ep_num, dir_in ? "IN" : "OUT", epid, mps);
	return true;
}

/* Split a buffer on 64K boundaries into Normal TRBs, ring EP doorbell, and poll for completion */
bool xhci_bulk_xfer(const struct usb_dev *ud, int dir_in, void *buf, uint32_t len) {
	if (!ud || !ud->hc || !buf || len == 0) {
		return false;
	}
	struct xhci_hc *hc = (struct xhci_hc *)ud->hc;
	struct xhci_slot_state *ss = xhci_ss(hc, ud->slot_id);
	if (!ss) return false;

	/* Find which EP we’re using by direction. For now, prefer EP2 if its MPS is set, else EP1. */
	uint8_t ep_num = 2;
	uint16_t mps = dir_in ? ss->bulk_in_mps : ss->bulk_out_mps;
	if (mps == 0) {
		/* fallback to EP1 if that was configured as bulk */
		ep_num = 1;
		mps = dir_in ? ss->bulk_in_mps : ss->bulk_out_mps;
		if (mps == 0) {
			dprintf("xhci: bulk_xfer no configured bulk EP (slot=%u dir=%d)\n", ud->slot_id, dir_in);
			return false;
		}
	}
	uint8_t epid = xhci_epid_from(ep_num, dir_in);
	struct xhci_ring *r = dir_in ? &ss->bulk_in_tr : &ss->bulk_out_tr;

	/* Build Normal TRBs over the buffer, respecting 64K segment boundary rule */
	uint64_t cur = (uint64_t)(uintptr_t)buf;
	uint32_t remain = len;
	int trb_count = 0;

	while (remain) {
		uint64_t next_boundary = (cur + 0x10000ull) & ~0xFFFFull;
		uint32_t chunk = (remain < (uint32_t)(next_boundary - cur)) ? remain : (uint32_t)(next_boundary - cur);

		struct trb *t = ring_push(r);
		t->lo = (uint32_t)(cur & 0xFFFFFFFFu);
		t->hi = (uint32_t)(cur >> 32);
		t->sts = chunk;
		/* For bulk, do not set ISP; set CH for multi-TRB TD, IOC only on the last TRB below */
		t->ctrl = TRB_SET_TYPE(TRB_NORMAL) | TRB_CH | (r->cycle ? TRB_CYCLE : 0) | (dir_in ? TRB_DIR : 0);

		cur += chunk;
		remain -= chunk;
		trb_count++;
	}

	/* Mark last TRB as IOC and clear CH on it */
	if (trb_count > 0) {
		uint32_t idx_last = (r->enqueue == 0) ? (r->num_trbs - 2) : (r->enqueue - 1); /* -1 from current enqueue; avoid link TRB */
		struct trb *last = &r->base[idx_last];
		last->ctrl = (last->ctrl & ~TRB_CH) | TRB_IOC | (dir_in ? TRB_DIR : 0);
	}

	/* Ring doorbell for this slot/EPID */
	mmio_write32(hc->db + 4u * ud->slot_id, epid);

	/* Poll for the Transfer Event(s) of this TD */
	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;
	uint64_t erdp_cur = mmio_read64(ir0 + IR_ERDP) & ~0x7ull;
	mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));

	uint64_t deadline = get_ticks() + 2000; /* up to ~2s for I/O */
	uint32_t completed = 0;
	for (;;) {
		int progressed = 0;
		for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
			struct trb *e = &hc->evt.base[i];
			if ((e->lo | e->hi | e->sts | e->ctrl) == 0u) {
				continue;
			}
			uint8_t type = (uint8_t)((e->ctrl >> 10) & 0x3F);
			if (type == TRB_TRANSFER_EVENT) {
				uint8_t e_slot = (uint8_t)(e->ctrl & 0xFF);
				uint8_t e_epid = (uint8_t)((e->ctrl >> 16) & 0x1F);
				if (e_slot == ud->slot_id && e_epid == epid) {
					uint8_t cc = (uint8_t)((e->sts >> 24) & 0xFF);
					/* consume */
					memset(e, 0, sizeof(*e));
					erdp_cur = (erdp_cur + 16) & ~0x7ull;
					mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));
					progressed = 1;
					completed = 1;
					if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET) {
						/* success: clear EHB and return */
						mmio_write64(ir0 + IR_ERDP, erdp_cur);
						return true;
					} else {
						dprintf("xhci: bulk TE cc=%u (slot=%u epid=%u)\n", cc, e_slot, e_epid);
						mmio_write64(ir0 + IR_ERDP, erdp_cur);
						return false;
					}
				}
			}

			/* unrelated event: keep ERDP moving */
			memset(e, 0, sizeof(*e));
			erdp_cur = (erdp_cur + 16) & ~0x7ull;
			mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));
			progressed = 1;
		}

		if (completed) {
			break;
		}
		if (!progressed) {
			if ((int64_t)(get_ticks() - deadline) > 0) {
				break;
			}
			__builtin_ia32_pause();
		}
	}

	mmio_write64(ir0 + IR_ERDP, erdp_cur);
	dprintf("xhci: bulk_xfer timeout\n");
	return false;
}
