#include <kernel.h>
#include "usb_core.h"
#include "usb_xhci.h"
#include "mmio.h"

static struct xhci_hc* g_xhci; /* single controller */

static void xhci_deliver_ep1in(struct xhci_hc *hc, const struct trb *ev, bool deliver_to_driver);

static inline uint8_t trb_cycle_bit(const struct trb *t) {
	return (uint8_t)(t->ctrl & TRB_CYCLE);
}

static inline uint8_t trb_get_type(const struct trb *e) {
	return (uint8_t)((e->ctrl >> 10) & 0x3F);
}

static inline uint8_t trb_get_epid(const struct trb *e) {
	return (uint8_t)((e->ctrl >> 16) & 0x1F);
}

static inline uint32_t trb_get_evtl(const struct trb *e)  {
	return (e->sts & 0x00FFFFFFu);
}

static inline uint8_t trb_get_cc(const struct trb *e)
{
	return (uint8_t)((e->sts >> 24) & 0xFF);
}

static inline uint32_t trb_get_bytes_remaining(const struct trb *e)
{
	return (e->sts & 0x00FFFFFFu);
}

static inline uint8_t trb_get_slotid(const struct trb *e)
{
	/* SlotID is bits [7:0] of control dword per xHCI spec */
	return (uint8_t)(e->ctrl & 0xFF);
}

static void xhci_handle_unrelated_transfer_event(struct xhci_hc *hc, struct trb *e)
{
	if (!hc || !e) return;

	switch (trb_get_type(e)) {
		case TRB_TRANSFER_EVENT: {
			/* If this is our INT IN endpoint, deliver to HID and re-arm */
			uint8_t epid = trb_get_epid(e);
			if (epid == EPID_EP1_IN && hc->dev.int_cb && hc->dev.int_buf && hc->dev.int_pkt_len) {
				xhci_deliver_ep1in(hc, e, false);
			}
			break;
		}

			/* We don’t act on other event types here; callers still consume & advance ERDP. */
		case TRB_CMD_COMPLETION:
		case TRB_PORT_STATUS:
		case TRB_HOST_CONTROLLER_EVENT:
		default:
			break;
	}
}


/* Decode a Transfer Event TRB -> slot_id, endpoint_id, cc, bytes (EVTL). */
static inline void xhci_decode_xfer_evt(const struct trb *e, uint8_t *slot_id, uint8_t *ep_id, uint8_t *cc, uint32_t *bytes)
{
	/* Dword3: [31:24]=CC, [23:17]=rsvd, [16:8]=EPID, [7:0]=SlotID */
	uint32_t ctrl  = e->ctrl;
	uint32_t sts   = e->sts;
	*cc     = (uint8_t)((sts  >> 24) & 0xFF);
	*ep_id  = (uint8_t)((ctrl >> 16) & 0x1F);
	*slot_id= (uint8_t)(ctrl & 0xFF);
	*bytes  = (sts & 0x00FFFFFFu); /* EVTL: bytes remaining */
}

/* ring_init with debug - set Toggle Cycle (TC) like libpayload */
static void ring_init(struct xhci_ring *r, size_t bytes, uint64_t phys, void *virt) {
	dprintf("xhci.dbg: ring_init bytes=%lu phys=%lx virt=%p\n", bytes, phys, virt);

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

	dprintf("xhci.dbg:   TRBs=%u link_trb@%u lo=%08x hi=%08x ctrl=%08x\n", r->num_trbs, r->num_trbs - 1, link->lo, link->hi, link->ctrl);
}

// assumes: slot (last index) is pre-initialized as LINK TRB:
//   link->lo = ring_phys;                 // target
//   link->ctrl = TRB_SET_TYPE(TRB_LINK) | TRB_TOGGLE;  // TC=1
//   r->cycle = 1; r->enqueue = 0;

static inline struct trb *ring_push(struct xhci_ring *r)
{
	struct trb *cur = &r->base[r->enqueue];

	/* advance producer */
	r->enqueue++;
	dprintf("enqueue=%u num_trbs=%u\n", r->enqueue, r->num_trbs);

	if (r->enqueue == r->num_trbs) {
		dprintf("wrap LINK\n");
		struct trb *link = &r->base[r->num_trbs - 1];

		/* present LINK to HW with current producer cycle */
		link->ctrl = (link->ctrl & ~TRB_CYCLE) | (r->cycle ? TRB_CYCLE : 0);

		/* wrap to start */
		r->enqueue = 0;

		/* flip producer cycle if LINK.TC set */
		if (link->ctrl & TRB_TOGGLE)
			r->cycle ^= 1u;

		/* hand back TRB[0] and advance to 1 so next push gets TRB[1] */
		cur = &r->base[r->enqueue++];
	}

	/* clear only the TRB we’re returning (never the LINK) */
	memset(cur, 0, sizeof(*cur));
	return cur;
}
static int xhci_cmd_submit_wait(struct xhci_hc *hc, struct trb *cmd_trb, uint64_t *out_cc_trb_lohi) {
	(void)out_cc_trb_lohi;

	uint64_t cmd_phys = hc->cmd.phys +
			    (uint64_t)((uintptr_t)cmd_trb - (uintptr_t)hc->cmd.base);

	dprintf("xhci.dbg: cmd_submit dbell0, trb_virt=%lx trb_phys=%lx ctrl=%08x\n", (uintptr_t)cmd_trb, cmd_phys, cmd_trb->ctrl);

	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;

	/* Save + mask IE for polled wait */
	uint32_t iman_old = mmio_read32(ir0 + IR_IMAN);
	mmio_write32(ir0 + IR_IMAN, (uint32_t)(iman_old & ~IR_IMAN_IE));

	/* Claim the ring: set EHB (bit3) once before polling */
	uint64_t erdp_start = mmio_read64(ir0 + IR_ERDP) & ~0x7ull;
	mmio_write64(ir0 + IR_ERDP, erdp_start | (1ull << 3));

	/* Doorbell command ring */
	mmio_write32(hc->db + 0, 0);

	uint64_t deadline = get_ticks() + 50;
	uint64_t erdp_cur = erdp_start;

	for (;;) {
		for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
			struct trb *e = &hc->evt.base[i];
			if ((e->lo | e->hi | e->sts | e->ctrl) == 0u) continue;

			uint32_t type = (e->ctrl >> 10) & 0x3Fu;
			uint64_t evt_ptr = ((uint64_t)e->hi << 32) | e->lo;

			if (type == TRB_CMD_COMPLETION) {
				uint32_t cc = (e->sts >> 24) & 0xFFu;
				dprintf("xhci.dbg:   evt[%u] type=%u lo=%08x hi=%08x sts=%08x cc=%02x match=%s\n",
					i, type, e->lo, e->hi, e->sts, cc, (evt_ptr == cmd_phys ? "yes" : "no"));

				if (evt_ptr == cmd_phys) {
					/* consume and bump ERDP by one TRB (keep EHB while in poll) */
					xhci_handle_unrelated_transfer_event(hc, e);
					memset(e, 0, sizeof(*e));
					erdp_cur = (erdp_cur + 16) & ~0x7ull;
					mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));

					/* leave with EHB CLEARED and IE restored */
					mmio_write64(ir0 + IR_ERDP, erdp_cur);                    /* clear EHB */
					mmio_write32(ir0 + IR_IMAN, (uint32_t)(iman_old | IR_IMAN_IE));

					if (cc != 0x01u) {
						dprintf("xhci.dbg:   command failed, cc=%02x\n", cc);
						return 0;
					}
					return 1;
				}
			}

			/* eat unrelated event under poll to keep ERDP in sync */
			xhci_handle_unrelated_transfer_event(hc, e);
			memset(e, 0, sizeof(*e));
			erdp_cur = (erdp_cur + 16) & ~0x7ull;
			mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));
		}

		if ((int64_t)(get_ticks() - deadline) > 0) break;
		__builtin_ia32_pause();
	}

	/* timeout: clear EHB and restore IE */
	mmio_write64(ir0 + IR_ERDP, erdp_cur);
	mmio_write32(ir0 + IR_IMAN, (uint32_t)(iman_old | IR_IMAN_IE));
	dprintf("xhci: command timeout\n");
	return 0;
}

static struct trb *xhci_cmd_begin(struct xhci_hc *hc) {
	struct trb *t = ring_push(&hc->cmd);
	t->ctrl |= hc->cmd.cycle ? TRB_CYCLE : 0;
	return t;
}

static void xhci_irq_sanity_dump(struct xhci_hc *hc, const char *tag)
{
	return;
	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;

	uint32_t usbcmd = mmio_read32(hc->op + XHCI_USBCMD);
	uint32_t usbsts = mmio_read32(hc->op + XHCI_USBSTS);
	uint32_t iman   = mmio_read32(ir0 + IR_IMAN);
	uint32_t imod   = mmio_read32(ir0 + IR_IMOD);
	uint64_t erdp   = mmio_read64(ir0 + IR_ERDP);

	dprintf("xhci.irq[%s]: USBCMD=%08x(INTE=%u) USBSTS=%08x(EINT=%u) "
		"IMAN=%08x(IE=%u IP=%u) IMOD=%08x(I=%u C=%u) ERDP=%llx(EHB=%u)\n",
		tag,
		usbcmd, (usbcmd & USBCMD_INTE) ? 1 : 0,
		usbsts, (usbsts & USBSTS_EINT) ? 1 : 0,
		iman, (iman & IR_IMAN_IE) ? 1 : 0, (iman & IR_IMAN_IP) ? 1 : 0,
		imod, (unsigned)(imod & 0xFFFF), (unsigned)(imod >> 16),
		(unsigned long long)erdp, (unsigned)((erdp >> 3) & 1));
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
			__builtin_ia32_pause();
		}
		dprintf("xhci.dbg:   halted, USBSTS=%08x\n", mmio_read32(hc->op + XHCI_USBSTS));
	}

	/* HCRST */
	mmio_write32(hc->op + XHCI_USBCMD, USBCMD_HCRST);
	uint64_t until = get_ticks() + 100;
	while ((mmio_read32(hc->op + XHCI_USBCMD) & USBCMD_HCRST) && get_ticks() < until) {
		__builtin_ia32_pause();
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
	hc->evt.dequeue = 0;   /* software consumer index for the event ring */
	hc->evt.ccs     = 1;   /* consumer cycle state starts at 1 per xHCI */

	hc->erst = (struct erst_entry *) kmalloc_aligned(64, 64);
	memset(hc->erst, 0, 64);
	hc->erst[0].ring_base = er_phys;
	hc->erst[0].size = 256;
	hc->erst_phys = (uint64_t)(uintptr_t) hc->erst;

	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;
	mmio_write32(ir0 + IR_ERSTSZ, 1);
	mmio_write64(ir0 + IR_ERSTBA, hc->erst_phys);
	mmio_write64(ir0 + IR_ERDP, er_phys | (1ull << 3));

	mmio_write32(ir0 + IR_IMOD, 0);
	mmio_write32(ir0 + IR_IMAN, IR_IMAN_IE);

	dprintf("xhci.dbg:   ERSTSZ=1 ERSTBA=%lx ERDP=%lx IMOD=64 IMAN=%08x\n", hc->erst_phys, (uint64_t)(er_phys | (1ull << 3)), mmio_read32(ir0 + IR_IMAN));

	/* CONFIG, RS+INTE */
	mmio_write32(hc->op + XHCI_CONFIG, hc->max_slots ? hc->max_slots : 8);
	mmio_write32(hc->op + XHCI_USBCMD, USBCMD_RS | USBCMD_INTE);

	until = get_ticks() + 20;
	while ((mmio_read32(hc->op + XHCI_USBSTS) & USBSTS_HCH) && get_ticks() < until) {
		__builtin_ia32_pause();
	}

	uint32_t sts2 = mmio_read32(hc->op + XHCI_USBSTS);
	uint32_t cmd2 = mmio_read32(hc->op + XHCI_USBCMD);
	dprintf("xhci.dbg:   after start USBCMD=%08x USBSTS=%08x\n", cmd2, sts2);
	return 1;
}

/* ---- EP context helpers ---- */
static void ctx_program_ep0(struct ep_ctx *e, uint16_t mps, uint64_t tr_deq_phys) {
	/* CErr=3 in dword0; Type+MPS belong in dword1 */
	e->dword0 = (3u << 1);                                /* CErr=3 */
	e->dword1 = ((uint32_t)mps << 16) | (4u << 8);        /* Type=4 (Control) */
	e->deq    = (tr_deq_phys | 1u);                       /* DCS=1 */
	e->dword4 = 8u;                                       /* Avg TRB len */
}

/* Build one EP0 control TD like libpayload (no goto version):
 *  SETUP(IDT, IOC) -> [DATA... (CH=1, split at 64KiB)] -> EVENT_DATA(IOC) -> STATUS(IOC)
 * Then wait for 2 (no data) or 3 (with data) transfer events before returning.
 */
static int xhci_ctrl_build_and_run(struct xhci_hc *hc, uint8_t slot_id,
				   const uint8_t *setup8, void *data, uint16_t len, int dir_in)
{
	if (!hc || !setup8 || slot_id == 0) return 0;

	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;

	/* Save + mask IE for a polled EP0 TD window, set EHB once */
	uint32_t iman_old = mmio_read32(ir0 + IR_IMAN);
	mmio_write32(ir0 + IR_IMAN, (uint32_t)(iman_old & ~IR_IMAN_IE));
	uint64_t erdp_cur = mmio_read64(ir0 + IR_ERDP) & ~0x7ull;
	mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));

	/* One-exit cleanup pattern, no goto */
	do {
		/* Ensure EP0 transfer ring exists */
		if (!hc->dev.ep0_tr.base) {
			void *v = kmalloc_aligned(4096, 4096);
			if (!v) break;
			uint64_t p = (uint64_t)(uintptr_t)v;
			ring_init(&hc->dev.ep0_tr, 4096, p, v);
		}

		/* Optional bounce for DATA stage (as per libpayload) */
		int used_bounce = 0;
		if (len) {
			if (!hc->dev.ctrl_dma || hc->dev.ctrl_dma_sz < len) {
				if (hc->dev.ctrl_dma) { kfree_null(&hc->dev.ctrl_dma); hc->dev.ctrl_dma_phys = 0; hc->dev.ctrl_dma_sz = 0; }
				hc->dev.ctrl_dma_sz  = len;
				hc->dev.ctrl_dma     = (uint8_t *) kmalloc_aligned(len, 64);
				if (!hc->dev.ctrl_dma) break;
				hc->dev.ctrl_dma_phys = (uint64_t)(uintptr_t) hc->dev.ctrl_dma;
			}
			if (!dir_in) memcpy(hc->dev.ctrl_dma, data, len);
			used_bounce = 1;
		}

		/* === SETUP (IDT, IOC) === */
		struct trb *t_setup = ring_push(&hc->dev.ep0_tr);
		t_setup->lo   = *(const uint32_t *)&setup8[0];
		t_setup->hi   = *(const uint32_t *)&setup8[4];
		uint32_t trt  = len ? (dir_in ? 2u : 3u) : 0u;       /* §4.11.2.1 */
		t_setup->sts  = (trt << 16) | 8u;
		t_setup->ctrl = TRB_SET_TYPE(TRB_SETUP_STAGE) | TRB_IDT | TRB_IOC |
				(hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);

		/* === DATA (optional; CH=1; split on 64KiB) === */
		if (len) {
			uint64_t cur   = used_bounce ? hc->dev.ctrl_dma_phys
						     : (uint64_t)(uintptr_t)data;
			uint32_t remain = len;
			while (remain) {
				uint64_t next_boundary = (cur + 0x10000ull) & ~0xFFFFull;
				uint32_t chunk = (remain < (uint32_t)(next_boundary - cur))
						 ? remain : (uint32_t)(next_boundary - cur);

				struct trb *t = ring_push(&hc->dev.ep0_tr);
				t->lo   = (uint32_t)(cur & 0xFFFFFFFFu);
				t->hi   = (uint32_t)(cur >> 32);
				t->sts  = chunk;                                        /* TL */
				t->ctrl = TRB_SET_TYPE(TRB_DATA_STAGE) |
					  (dir_in ? TRB_DIR : 0) |
					  TRB_CH |                                              /* chain */
					  (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);

				cur    += chunk;
				remain -= chunk;
			}

			/* EVENT_DATA with IOC to delimit TD (mirrors libpayload) */
			struct trb *t_ev = ring_push(&hc->dev.ep0_tr);
			t_ev->lo   = (uint32_t)(uintptr_t)t_ev; /* for debug only */
			t_ev->hi   = 0;
			t_ev->sts  = 0;
			t_ev->ctrl = TRB_SET_TYPE(TRB_EVENT_DATA) | TRB_IOC |
				     (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);
		}

		/* === STATUS (IOC; opposite DIR to DATA; IN if no DATA) === */
		struct trb *t_status = ring_push(&hc->dev.ep0_tr);
		t_status->lo = 0; t_status->hi = 0; t_status->sts = 0;
		const int status_in = (len == 0) ? 1 : (!dir_in);
		t_status->ctrl = TRB_SET_TYPE(TRB_STATUS_STAGE) |
				 (status_in ? TRB_DIR : 0) | TRB_IOC |
				 (hc->dev.ep0_tr.cycle ? TRB_CYCLE : 0);

		/* Ring EP0 doorbell */
		mmio_write32(hc->db + 4u * slot_id, EPID_CTRL);

		/* === Wait for 2 (no data) or 3 (with data) transfer events === */
		const uint32_t target_events = 2u + (len ? 1u : 0u);
		uint32_t got_events = 0u;
		uint64_t deadline = get_ticks() + 100; /* ~100ms */

		for (;;) {
			int progressed = 0;
			for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
				struct trb *e = &hc->evt.base[i];
				if ((e->lo | e->hi | e->sts | e->ctrl) == 0u) continue;

				uint32_t type = (e->ctrl >> 10) & 0x3Fu;

				if (type == TRB_TRANSFER_EVENT) {
					/* For IN data, copy bounce once we see first completion */
					if (len && dir_in && hc->dev.ctrl_dma)
						memcpy(data, hc->dev.ctrl_dma, len);

					xhci_handle_unrelated_transfer_event(hc, e);
					memset(e, 0, sizeof(*e));
					erdp_cur = (erdp_cur + 16) & ~0x7ull;
					mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));

					got_events++;
					progressed = 1;
					if (got_events >= target_events) {
						/* success path: clear EHB, restore IE, return */
						mmio_write64(ir0 + IR_ERDP, erdp_cur);
						mmio_write32(ir0 + IR_IMAN, (uint32_t)(iman_old | IR_IMAN_IE));
						return 1;
					}
					continue;
				}

				/* unrelated event during polled window: consume to keep ERDP in sync */
				xhci_handle_unrelated_transfer_event(hc, e);
				memset(e, 0, sizeof(*e));
				erdp_cur = (erdp_cur + 16) & ~0x7ull;
				mmio_write64(ir0 + IR_ERDP, erdp_cur | (1ull << 3));
				progressed = 1;
			}

			if (!progressed) {
				if ((int64_t)(get_ticks() - deadline) > 0) break;
				__builtin_ia32_pause();
			}
		}

		/* timed out */
		break;
	} while (0);

	/* Common cleanup on failure: clear EHB and restore IE */
	mmio_write64(ir0 + IR_ERDP, erdp_cur);
	mmio_write32(ir0 + IR_IMAN, (uint32_t)(iman_old | IR_IMAN_IE));
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
	while (get_ticks() < end_wait) { __builtin_ia32_pause(); }

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
	uint8_t __attribute__((aligned(64))) setup_dev8[8] = { 0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 8, 0x00 };
	uint8_t __attribute__((aligned(64))) dev_desc[256];
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

	/* --- Assign the address now (BSR=0) so the slot moves to Addressed state --- */
	hc->dev.ic->add_flags  = 0x00000003u;          /* A0 | A1 (slot + ep0) */
	hc->dev.ic->drop_flags = 0;
	ctx_program_ep0(&hc->dev.ic->ep0, mps, hc->dev.ep0_tr.phys);

	struct trb *ad2 = xhci_cmd_begin(hc);
	ad2->lo = (uint32_t)(hc->dev.ic_phys & 0xFFFFFFFFu);
	ad2->hi = (uint32_t)(hc->dev.ic_phys >> 32);
	ad2->ctrl |= TRB_SET_TYPE(TRB_ADDRESS_DEVICE) | ((uint32_t)slot_id << 24); /* BSR=0 */
	dprintf("xhci.dbg:   ADDRESS_DEVICE (assign) slot=%u trb=%08x\n",
		slot_id, (uint32_t)(uintptr_t)ad2);
	if (!xhci_cmd_submit_wait(hc, ad2, NULL)) {
		kfree_null(&devctx);
		return 0;
	}
	dprintf("xhci.dbg:   ADDRESS_DEVICE (assign) ok\n");

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

	/* Parse first interface descriptor (class/subclass/proto) */
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

	/* Discover first Interrupt IN endpoint on interface 0 alt 0. */
	uint8_t  found_int_in = 0;
	uint8_t  int_ep_num = 1;      /* default to EP1 IN if not found */
	uint16_t int_ep_mps = 8;      /* boot kbd typical */
	uint8_t  int_ep_interval = 1; /* default */

	{
		uint8_t cur_if = 0xFF, cur_alt = 0xFF;
		for (uint16_t off = 9; off + 2 <= total_len; ) {
			uint8_t len = cfg_buf[off];
			uint8_t typ = cfg_buf[off + 1];
			if (len < 2) break;

			if (typ == 0x04 && len >= 9) {
				/* interface descriptor */
				cur_if  = cfg_buf[off + 2];
				cur_alt = cfg_buf[off + 3];
			} else if (typ == 0x05 && len >= 7) {
				/* endpoint descriptor */
				uint8_t  addr = cfg_buf[off + 2];  /* bEndpointAddress */
				uint8_t  attr = cfg_buf[off + 3];  /* bmAttributes */
				uint16_t wMax = (uint16_t)cfg_buf[off + 4] | ((uint16_t)cfg_buf[off + 5] << 8);
				uint8_t  intr = cfg_buf[off + 6];  /* bInterval */

				if (cur_if == 0 && cur_alt == 0 &&
				    (attr & 0x03) == 3 /* Interrupt */ &&
				    (addr & 0x80)      /* IN */) {
					int_ep_num      = (uint8_t)(addr & 0x0F);
					int_ep_mps      = wMax;
					int_ep_interval = intr ? intr : 1;
					found_int_in    = 1;
					break;
				}
			}
			off = (uint16_t)(off + len);
		}
	}

	/* Stash the discovered INT-IN parameters for the arming routine. */
	hc->dev.int_ep_num      = int_ep_num;
	hc->dev.int_ep_mps      = int_ep_mps;
	hc->dev.int_ep_interval = int_ep_interval;

	dprintf("xhci.dbg:   INT-IN ep discovery: num=%u mps=%u interval=%u (found=%u)\n",
		int_ep_num, int_ep_mps, int_ep_interval, found_int_in);

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

static void xhci_deliver_ep1in(struct xhci_hc *hc, const struct trb *e, bool deliver_to_driver)
{
	/* Sanity: must be our current device and EP1 IN (EPID=3). */
	uint8_t slot_id = trb_get_slotid(e);
	uint8_t epid    = trb_get_epid(e);
	if (slot_id != hc->dev.slot_id || epid != EPID_EP1_IN)
		return;

	if (!hc->dev.int_cb || !hc->dev.int_buf || !hc->dev.int_pkt_len)
		return;

	uint8_t  cc    = trb_get_cc(e);
	uint32_t rem   = trb_get_bytes_remaining(e);
	uint32_t req   = hc->dev.int_pkt_len;
	uint32_t xfer;

	/* Only deliver on Success or Short Packet. Others can be handled if needed. */
	if (cc == CC_SUCCESS || cc == CC_SHORT_PACKET) {
		/* Actual bytes = requested - remaining; guard against weird reports. */
		xfer = (rem <= req) ? (req - rem) : req;
		if (xfer == 0) xfer = req;         /* many boot keyboards always send 8 */

		if (deliver_to_driver) {
			struct usb_dev ud = {0};
			ud.hc = hc;
			ud.slot_id = hc->dev.slot_id;

			/* Up-call into HID with the bytes we actually got. */
			hc->dev.int_cb(&ud, hc->dev.int_buf, (uint16_t) xfer);
		}

		/* Re-arm exactly one new Normal TRB to keep a steady queue depth. */
		struct trb *n = ring_push(&hc->dev.int_in_tr);
		n->lo   = (uint32_t)(hc->dev.int_buf_phys & 0xFFFFFFFFu);
		n->hi   = (uint32_t)(hc->dev.int_buf_phys >> 32);
		n->sts  = hc->dev.int_pkt_len;
		n->ctrl = TRB_SET_TYPE(TRB_NORMAL) | TRB_IOC | TRB_ISP | (hc->dev.int_in_tr.cycle ? TRB_CYCLE : 0);

		/* Ring EP1 IN doorbell (per-slot doorbell, target = EPID). */
		mmio_write32(hc->db + 4u * hc->dev.slot_id, EPID_EP1_IN);
	} else {
		/* Optional: recover on errors (stall -> stop/reset EP) */
		dprintf("xhci.dbg: EP1-IN TE cc=%u rem=%u (ignored)\n", cc, rem);
	}
}

static uint64_t xhci_ints;

static void xhci_isr(uint8_t isr, uint64_t error, uint64_t irq, void *opaque)
{
	dprintf("xhci_isr %lu\n", xhci_ints++);
	struct xhci_hc *hc = (struct xhci_hc *)opaque;
	if (!hc || !hc->cap) return;

	xhci_irq_sanity_dump(hc, "ISR-enter");

	/* Ack controller summary (RW1C) */
	uint32_t sts = mmio_read32(hc->op + XHCI_USBSTS);
	if (sts & USBSTS_EINT) {
		mmio_write32(hc->op + XHCI_USBSTS, USBSTS_EINT);
	}

	volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;
	uint64_t erdp_cur = mmio_read64(ir0 + IR_ERDP) & ~0x7ull;

	/* IMPORTANT: ensure EHB is CLEAR so future interrupts can retrigger */
	mmio_write64(ir0 + IR_ERDP, erdp_cur);  /* bit3=0 */

	for (uint32_t i = 0; i < hc->evt.num_trbs; i++) {
		struct trb *e = &hc->evt.base[i];
		if ((e->lo | e->hi | e->sts | e->ctrl) == 0u) continue;

		uint8_t type = trb_get_type(e);
		//dprintf("xhci.dbg: ISR evt[%u] type=%u lo=%08x hi=%08x sts=%08x\n", i, type, e->lo, e->hi, e->sts);

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
		memset(e, 0, sizeof(*e));
		erdp_cur = (erdp_cur + 16) & ~0x7ull;
		dprintf("erdp_cur=%lu\n", erdp_cur);
		mmio_write64(ir0 + IR_ERDP, erdp_cur);
	}

	/* W1C IMAN.IP; keep IE as-is */
	uint32_t iman = mmio_read32(ir0 + IR_IMAN);
	if (iman & IR_IMAN_IP) {
		mmio_write32(ir0 + IR_IMAN, (uint32_t)(iman | IR_IMAN_IP));
	}

	xhci_irq_sanity_dump(hc, "ISR-exit");
}

int xhci_arm_int_in(struct usb_dev *ud, uint16_t pkt_len, xhci_int_in_cb cb) {
	dprintf("xhci.dbg: ARM INT-IN enter ud=%p hc=%p slot_id=%u pkt_len=%u cb=%p\n", ud, ud ? ud->hc : 0, ud ? ud->slot_id : 0, (unsigned)pkt_len, cb);

	if (!ud || !ud->hc || !pkt_len || !cb) {
		dprintf("xhci.dbg: ARM INT-IN early-exit (arg check) ud=%p hc=%p pkt_len=%u cb=%p\n",
			ud, ud ? ud->hc : 0, (unsigned)pkt_len, cb);
		return 0;
	}
	struct xhci_hc *hc = (struct xhci_hc *) ud->hc;

	/* Use discovered INT-IN from config (fallbacks retained). */
	uint8_t  ep_num       = hc->dev.int_ep_num      ? hc->dev.int_ep_num      : 1;
	uint16_t ep_mps       = hc->dev.int_ep_mps      ? hc->dev.int_ep_mps      : 8;
	uint8_t  ep_bInterval = hc->dev.int_ep_interval ? hc->dev.int_ep_interval : 1;

	/* Only EP1 IN is supported in this driver (EPID==3). */
	if (ep_num != 1) {
		uint8_t epid = (uint8_t)(2u * ep_num + 1u);
		dprintf("xhci.dbg: WARNING: device INT-IN at EP%u (EPID=%u), driver only supports EP1 IN; not arming.\n", ep_num, epid);
		return 0;
	}

	/* Ensure TR for EP1 IN */
	if (!hc->dev.int_in_tr.base) {
		void *v = kmalloc_aligned(4096, 4096);
		if (!v) {
			dprintf("xhci.dbg: INT-IN TR alloc fail\n");
			return 0;
		}
		uint64_t p = (uint64_t)(uintptr_t)v;
		dprintf("xhci.dbg: INT-IN TR alloc v=%p phys=%lx\n", v, p);
		ring_init(&hc->dev.int_in_tr, 4096, p, v);
		dprintf("xhci.dbg: INT-IN TR init base=%p phys=%lx cycle=%u\n",
			hc->dev.int_in_tr.base, hc->dev.int_in_tr.phys,
			(unsigned)hc->dev.int_in_tr.cycle);
	} else {
		dprintf("xhci.dbg: INT-IN TR already init base=%p phys=%lx cycle=%u\n",
			hc->dev.int_in_tr.base, hc->dev.int_in_tr.phys,
			(unsigned)hc->dev.int_in_tr.cycle);
	}

	/* Ensure report buffer */
	if (!hc->dev.int_buf) {
		hc->dev.int_buf = (uint8_t *) kmalloc_aligned(pkt_len, 64);
		if (!hc->dev.int_buf) {
			dprintf("xhci.dbg: INT-IN buf alloc fail (%u)\n", pkt_len);
			return 0;
		}
		hc->dev.int_buf_phys = (uint64_t)(uintptr_t) hc->dev.int_buf;
		dprintf("xhci.dbg: INT-IN buf alloc v=%p phys=%lx len=%u\n",
			hc->dev.int_buf, hc->dev.int_buf_phys, (unsigned)pkt_len);
	} else {
		dprintf("xhci.dbg: INT-IN buf reuse v=%p phys=%lx len=%u\n",
			hc->dev.int_buf, hc->dev.int_buf_phys, (unsigned)pkt_len);
	}
	hc->dev.int_cb = cb;
	hc->dev.int_pkt_len = pkt_len;

	/* Input Control Context: add Slot (A0) and EP1 IN (A3). */
	hc->dev.ic->drop_flags = 0;
	hc->dev.ic->add_flags  = 0x00000009; /* A0 | A3 */
	dprintf("xhci.dbg: ICC flags drop=%08x add=%08x ic=%p ic_phys=%lx\n",
		hc->dev.ic->drop_flags, hc->dev.ic->add_flags, hc->dev.ic, hc->dev.ic_phys);

	/* Copy current Device Slot Context -> Input Slot Context; set CE=3. */
	{
		uint64_t dphys = hc->dcbaa[ud->slot_id];
		volatile uint32_t *dcs = (volatile uint32_t *)(uintptr_t)dphys;  /* device slot ctx (8 dwords) */
		volatile uint32_t *isc = (volatile uint32_t *)&hc->dev.ic->slot; /* input  slot ctx (8 dwords) */
		for (int k = 0; k < 8; k++) isc[k] = dcs[k];
		/* CE = highest context ID present (3 => EP1 IN included) */
		isc[0] = (isc[0] & ~(0x1Fu << 27)) | (3u << 27);
		dprintf("xhci.dbg: slot.dword0(copy+CE)=%08x (CE=%u)\n",
			isc[0], (unsigned)((isc[0] >> 27) & 0x1F));
	}

	/* -------- Interval encoding per xHCI (libpayload semantics) --------
	 * HS/SS: INTVAL = bInterval - 1   (units: 125 µs exponent)
	 * FS/LS: INTVAL = bInterval       (units: frames)
	 * Bound the value like libpayload for FS/LS interrupt endpoints.
	 */
	uint32_t intval = 1;
	{
		/* speed code was captured in Slot Context dword2[3:0] during enumerate */
		uint8_t speed_code = (uint8_t)(hc->dev.ic->slot.dword2 & 0xF);

		if (speed_code >= 3) {
			/* High/Super speed */
			uint8_t bI = ep_bInterval ? ep_bInterval : 1;
			intval = (bI > 0) ? (uint32_t)(bI - 1) : 0u;
		} else {
			/* Full/Low speed */
			intval = ep_bInterval ? ep_bInterval : 1;
			if (intval < 3)      intval = 3;
			else if (intval > 11) intval = 11;
		}
		if (intval > 15) intval = 15;
	}

	/* Program EP1 IN context - field placement matches libpayload:
	 * DW0: CErr=3
	 * DW1: [31:16] MaxPacketSize, [15:8] Interval, [5:3] EPType (=7 Interrupt IN)
	 * TR Dequeue: ring phys | DCS
	 * DW4: AvgTRB Length (31:16), Max ESIT Payload (15:0)
	 */
	memset(&hc->dev.ic->ep1_in, 0, sizeof(hc->dev.ic->ep1_in));
	hc->dev.ic->ep1_in.dword0 = (3u << 1);

	hc->dev.ic->ep1_in.dword1 =
		((uint32_t)ep_mps << 16) |
		((intval & 0xFFu) << 8) |
		(7u << 3); /* EPType = 7 (Interrupt IN) in bits [5:3] */

	hc->dev.ic->ep1_in.deq = (hc->dev.int_in_tr.phys | 1u);

	{
		uint32_t avg_trb_len = 1024;               /* libpayload uses 1024 for INT */
		uint32_t max_esit    = ep_mps;             /* MPS * MBS (MBS defaults to 1 for HID) */
		hc->dev.ic->ep1_in.dword4 = (avg_trb_len << 16) | max_esit;
	}

	{
		const uint32_t *epd = (const uint32_t *)&hc->dev.ic->ep1_in;
		dprintf("xhci.dbg: EP1 IN ctx dwords: %08x %08x %08x %08x | %08x %08x %08x %08x (mps=%u bInterval=%u -> intval=%u)\n",
			epd[0], epd[1], epd[2], epd[3], epd[4], epd[5], epd[6], epd[7],
			ep_mps, ep_bInterval, (unsigned)intval);
	}

	dprintf("xhci.dbg: CONFIGURE_EP add_flags=%08x slot.dword0=%08x (CE=%u)\n",
		hc->dev.ic->add_flags, hc->dev.ic->slot.dword0,
		(hc->dev.ic->slot.dword0 >> 27) & 0x1F);

	/* Issue Configure Endpoint (add EP1 IN) */
	struct trb *ce = xhci_cmd_begin(hc);
	ce->lo   = (uint32_t)(hc->dev.ic_phys & 0xFFFFFFFFu);
	ce->hi   = (uint32_t)(hc->dev.ic_phys >> 32);
	ce->ctrl |= TRB_SET_TYPE(TRB_CONFIGURE_EP) | ((uint32_t)ud->slot_id << 24);
	dprintf("xhci.dbg: ring_push for CONFIGURE_EP trb=%08x ic_phys_lo=%08x ic_phys_hi=%08x ctrl=%08x slot=%u\n",
		(uint32_t)(uintptr_t)ce,
		(uint32_t)(hc->dev.ic_phys & 0xFFFFFFFFu),
		(uint32_t)(hc->dev.ic_phys >> 32),
		ce->ctrl, ud->slot_id);

	if (!xhci_cmd_submit_wait(hc, ce, NULL)) {
		dprintf("xhci.dbg: CONFIGURE_EP timeout/fail\n");
		return 0;
	}
	dprintf("xhci.dbg: CONFIGURE_EP success\n");

	/* Pre-post two INT-IN TRBs (IOC|ISP) and ring once (libpayload style) */
	for (int k = 0; k < 2; ++k) {
		struct trb *n = ring_push(&hc->dev.int_in_tr);
		n->lo   = (uint32_t)(hc->dev.int_buf_phys & 0xFFFFFFFFu);
		n->hi   = (uint32_t)(hc->dev.int_buf_phys >> 32);
		n->sts  = hc->dev.int_pkt_len;
		n->ctrl = TRB_SET_TYPE(TRB_NORMAL) | TRB_ISP | TRB_IOC |
			  (hc->dev.int_in_tr.cycle ? TRB_CYCLE : 0);
	}
	mmio_write32(hc->db + 4u * ud->slot_id, EPID_EP1_IN);
	dprintf("xhci.dbg:   doorbell EP1 IN (slot=%u) tr.phys=%lx buf.phys=%lx (prepost=2)\n",
		ud->slot_id, hc->dev.int_in_tr.phys, hc->dev.int_buf_phys);

	/* Steady-state: moderation 0, clear EHB & IP, enable IE and INTE */
	{
		volatile uint8_t *ir0 = hc->rt + XHCI_RT_IR0;

		mmio_write32(ir0 + IR_IMOD, 64);

		uint32_t sts = mmio_read32(hc->op + XHCI_USBSTS);
		if (sts & USBSTS_EINT) mmio_write32(hc->op + XHCI_USBSTS, USBSTS_EINT);

		uint32_t iman = mmio_read32(ir0 + IR_IMAN);
		if (iman & IR_IMAN_IP) mmio_write32(ir0 + IR_IMAN, IR_IMAN_IP); /* W1C IP */
		mmio_write32(ir0 + IR_IMAN, (mmio_read32(ir0 + IR_IMAN) | IR_IMAN_IE));

		uint64_t erdp = mmio_read64(ir0 + IR_ERDP) & ~0x7ull;
		mmio_write64(ir0 + IR_ERDP, erdp); /* EHB clear */

		uint32_t cmd = mmio_read32(hc->op + XHCI_USBCMD) | USBCMD_INTE;
		mmio_write32(hc->op + XHCI_USBCMD, cmd);

		xhci_irq_sanity_dump(hc, "post-arm");
	}

	return 1;
}

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

	uint32_t irq_line = pci_read(p, PCI_INTERRUPT_LINE) & 0xFFu;
	uint32_t irq_pin  = pci_read(p, PCI_INTERRUPT_PIN) & 0xFFu;
	g_xhci->irq_line = (uint8_t) irq_line;
	g_xhci->irq_pin  = (uint8_t) irq_pin;

	//register_interrupt_handler(IRQ_START + irq_line, xhci_isr, p, g_xhci);
	pci_setup_interrupt("xhci", p, logical_cpu_id(), xhci_isr, g_xhci);
	dprintf("xhci: using legacy INTx IRQ=%u (PIN#%c)\n", irq_line, 'A' + (int)irq_pin - 1);

	struct usb_dev ud;
	if (!xhci_enumerate_first_device(g_xhci, &ud)) {
		dprintf("xhci: controller up, 0 devices present\n");
		dprintf("USB xHCI: controller ready (interrupts on), published 0 devices\n");
		return 1; /* host is fine; no device yet */
	}

	usb_core_device_added(&ud);
	dprintf("USB xHCI: controller ready (interrupts on), published 1 device\n");
	return 1;
}

/* find first USB controller by class code 0x0C03 and ensure xHCI (prog-if 0x30) */
void init_usb_xhci(void) {
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

	dprintf("USB: xHCI initialised at %u:%u.%u\n", usb.bus_num, usb.device_num, usb.function_num);
}