/* usb_msc.c
 *
 * Minimal USB Mass Storage (Bulk-Only Transport) class driver.
 * Implements: INQUIRY, TEST UNIT READY, READ CAPACITY(10), READ(10) LBA 0.
 *
 * House style aligned with existing HID driver.
 */

#include <kernel.h>

static bool get_config_descriptor_full(const struct usb_dev *ud, uint8_t *buf, uint16_t *out_len)
{
	uint8_t head9[9] __attribute__((aligned(64)));
	uint8_t setup9[8]  = {0x80,0x06,0x00,0x02,0x00,0x00,9,0x00};
	uint8_t setup_full[8];

	if (!usb_ctrl_get(ud, setup9[0], setup9[1], (uint16_t)(setup9[2] | (setup9[3] << 8)),
			  (uint16_t)(setup9[4] | (setup9[5] << 8)), head9, 9)) {
		return false;
	}
	uint16_t total = (uint16_t)head9[2] | ((uint16_t)head9[3] << 8);
	if (total > *out_len) total = *out_len;

	setup_full[0]=0x80; setup_full[1]=0x06; setup_full[2]=0x00; setup_full[3]=0x02;
	setup_full[4]=0x00; setup_full[5]=0x00; setup_full[6]=(uint8_t)(total & 0xFF); setup_full[7]=(uint8_t)(total >> 8);

	if (!usb_ctrl_get(ud, setup_full[0], setup_full[1], (uint16_t)(setup_full[2] | (setup_full[3] << 8)),
			  (uint16_t)(setup_full[4] | (setup_full[5] << 8)), buf, total)) {
		return false;
	}
	*out_len = total;
	return true;
}

static bool find_msc_interface_and_endpoints(const uint8_t *cfg, uint16_t len, uint8_t *iface, uint8_t *ep_bulk_out, uint16_t *mps_out, uint8_t *ep_bulk_in,  uint16_t *mps_in) {
	uint8_t cur_if = 0xFF, cur_alt = 0xFF;
	bool in_ok = false, out_ok = false;
	if (!cfg || len < 9) return false;

	for (uint16_t off = 9; off + 2 <= len;) {
		uint8_t dlen = cfg[off];
		uint8_t dtype = cfg[off + 1];
		if (dlen < 2) break;

		if (dtype == 0x04 && dlen >= 9) {
			/* interface */
			cur_if = cfg[off + 2];
			cur_alt = cfg[off + 3];
			uint8_t cls = cfg[off + 5], sub = cfg[off + 6], pro = cfg[off + 7];
			if (cls == USB_CLASS_MSC && sub == USB_SUBCLASS_SCSI && pro == USB_PROTO_BULK && cur_alt == 0) {
				if (*iface == 0xFF) *iface = cur_if; /* prefer first match */
			}
		} else if (dtype == 0x05 && dlen >= 7) {
			/* endpoint */
			if (*iface != 0xFF && cur_if == *iface && cur_alt == 0) {
				uint8_t addr = cfg[off + 2];
				uint8_t attr = cfg[off + 3];
				uint16_t wMax = (uint16_t)cfg[off + 4] | ((uint16_t)cfg[off + 5] << 8);
				if ((attr & 0x03) == 2) { /* bulk */
					if (addr & 0x80) {
						*ep_bulk_in = (uint8_t)(addr & 0x0F);
						*mps_in = wMax;
						in_ok = true;
					} else {
						*ep_bulk_out = (uint8_t)(addr & 0x0F);
						*mps_out = wMax;
						out_ok = true;
					}
				}
			}
		}
		off = (uint16_t)(off + dlen);
	}
	return (*iface != 0xFF) && in_ok && out_ok;
}

/* CBW/CSW exchange for one BOT command + optional data phase. */
static bool msc_exchange(const struct usb_dev *ud, const uint8_t *cmd, uint8_t cmd_len, void *data, uint32_t data_len, bool dir_in) {
	struct msc_cbw cbw;
	struct msc_csw csw;
	memset(&cbw, 0, sizeof(cbw));
	memset(&csw, 0, sizeof(csw));

	cbw.sig = CBW_SIG;
	cbw.tag = 0x12345678;
	cbw.data_len = data_len;
	cbw.flags = dir_in ? 0x80 : 0x00;
	cbw.lun = 0;
	cbw.cmd_len = cmd_len;
	memcpy(cbw.cmd, cmd, cmd_len);

	/* OUT: CBW over bulk OUT */
	if (!xhci_bulk_xfer(ud, 0, &cbw, sizeof(cbw))) {
		dprintf("msc: CBW OUT fail\n");
		return false;
	}

	/* Optional data phase */
	if (data_len) {
		if (dir_in) {
			if (!xhci_bulk_xfer(ud, 1, data, data_len)) {
				dprintf("msc: DATA IN fail\n");
				return false;
			}
		} else {
			if (!xhci_bulk_xfer(ud, 0, data, data_len)) {
				dprintf("msc: DATA OUT fail\n");
				return false;
			}
		}
	}

	/* IN: CSW over bulk IN */
	if (!xhci_bulk_xfer(ud, 1, &csw, sizeof(csw))) {
		dprintf("msc: CSW IN fail\n");
		return false;
	}
	if (csw.sig != CSW_SIG) {
		dprintf("msc: CSW bad sig %08x\n", csw.sig);
		return false;
	}
	if (csw.status != 0) {
		dprintf("msc: CSW status=%u residue=%u\n", csw.status, csw.residue);
		return false;
	}
	return true;
}

/* --- basic SCSI ops ----------------------------------------------------- */

static bool msc_inquiry(const struct usb_dev *ud)
{
	uint8_t cmd[6] = {0x12,0,0,0,36,0};
	uint8_t buf[36] __attribute__((aligned(64)));
	memset(buf, 0, sizeof(buf));

	if (!msc_exchange(ud, cmd, sizeof(cmd), buf, sizeof(buf), true)) {
		dprintf("msc: INQUIRY failed\n");
		return false;
	}
	dprintf("msc: INQUIRY vendor='%.8s' product='%.16s' rev='%.4s'\n",
		&buf[8], &buf[16], &buf[32]);
	return true;
}

static bool msc_test_unit_ready(const struct usb_dev *ud)
{
	uint8_t cmd[6] = {0x00,0,0,0,0,0};
	return msc_exchange(ud, cmd, sizeof(cmd), NULL, 0, true);
}

static bool msc_read_capacity(const struct usb_dev *ud, uint32_t *blocks_out, uint32_t *blk_len_out)
{
	uint8_t cmd[10] = {0x25,0,0,0,0,0,0,0,0,0};
	uint8_t buf[8] __attribute__((aligned(64)));
	memset(buf, 0, sizeof(buf));

	if (!msc_exchange(ud, cmd, sizeof(cmd), buf, sizeof(buf), true)) {
		dprintf("msc: READ CAPACITY(10) failed\n");
		return false;
	}
	uint32_t last_lba = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
	uint32_t blk_len  = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) | ((uint32_t)buf[6] << 8) | buf[7];

	if (blocks_out) *blocks_out = last_lba + 1;
	if (blk_len_out) *blk_len_out = blk_len;

	dprintf("msc: capacity blocks=%lu blk_len=%lu\n", (unsigned long)(last_lba + 1), (unsigned long)blk_len);
	return true;
}

static bool msc_read10(const struct usb_dev *ud, uint32_t lba, uint16_t blocks, void *buf, uint32_t bytes)
{
	uint8_t cmd[10] = {0x28,0,0,0,0,0,0,0,0,0};
	cmd[2] = (uint8_t)(lba >> 24);
	cmd[3] = (uint8_t)(lba >> 16);
	cmd[4] = (uint8_t)(lba >> 8);
	cmd[5] = (uint8_t)(lba);
	cmd[7] = (uint8_t)(blocks >> 8);
	cmd[8] = (uint8_t)(blocks);

	return msc_exchange(ud, cmd, sizeof(cmd), buf, bytes, true);
}

/* --- class hooks -------------------------------------------------------- */

static void msc_on_device_added(const struct usb_dev *ud)
{
	if (!ud) return;

	/* Claim only SCSI / BOT */
	if (!(ud->dev_class == USB_CLASS_MSC && ud->dev_subclass == USB_SUBCLASS_SCSI && ud->dev_proto == USB_PROTO_BULK)) {
		return;
	}

	dprintf("msc: attach VID:PID=%04x:%04x slot=%u iface=%u\n", ud->vid, ud->pid, ud->slot_id, ud->iface_num);

	/* Ensure configuration 1 (common on QEMU/most sticks) */
	if (!usb_ctrl_nodata(ud, 0x00, 0x09, 0x0001, 0x0000)) {
		dprintf("msc: SET_CONFIGURATION(1) failed\n");
		return;
	}

	/* Fetch config to discover endpoints for this interface. */
	uint8_t cfg[512] __attribute__((aligned(64)));
	uint16_t cfg_len = sizeof(cfg);
	if (!get_config_descriptor_full(ud, cfg, &cfg_len)) {
		dprintf("msc: failed to read config\n");
		return;
	}

	uint8_t iface = ud->iface_num; /* default 0; updated if we find exact match */
	uint8_t ep_out = 0xFF, ep_in = 0xFF;
	uint16_t mps_out = 0, mps_in = 0;

	iface = (iface == 0) ? 0xFF : iface; /* if HCD left 0, allow rediscovery */
	if (!find_msc_interface_and_endpoints(cfg, cfg_len, &iface, &ep_out, &mps_out, &ep_in, &mps_in)) {
		dprintf("msc: could not find bulk endpoints\n");
		return;
	}

	/* Open pipes (OUT, then IN). Store ep nums into slot state for xhci_bulk_xfer. */
	struct xhci_hc *hc = (struct xhci_hc *)ud->hc;
	struct xhci_slot_state *ss = &hc->slots[ud->slot_id];
	ss->bulk_out_num = ep_out;
	ss->bulk_in_num  = ep_in;

	if (!xhci_open_bulk_pipe(ud, ep_out, 0, mps_out)) return;
	if (!xhci_open_bulk_pipe(ud, ep_in,  1, mps_in))  return;

	/* Basic BOT bring-up */
	if (!msc_inquiry(ud)) return;

	/* TUR may fail until media ready — do a few retries. */
	bool ready = false;
	for (int i = 0; i < 5; i++) {
		if (msc_test_unit_ready(ud)) { ready = true; break; }
		sleep(50);
	}
	if (!ready) {
		dprintf("msc: not ready\n");
		return;
	}

	uint32_t blocks = 0, blk_len = 0;
	if (!msc_read_capacity(ud, &blocks, &blk_len)) return;

	/* Read first sector and dump a small prefix (visible proof we used the reply). */
	uint32_t to_read = (blk_len && blk_len <= 4096) ? blk_len : 512;
	uint8_t *sector0 = (uint8_t *)kmalloc_aligned(to_read, 64);
	if (!sector0) return;

	if (msc_read10(ud, 0, 1, sector0, to_read)) {
		dprintf("msc: LBA0 first 64 bytes:\n");
		dump_hex(sector0, (to_read < 64) ? to_read : 64);
	} else {
		dprintf("msc: READ(10) LBA0 failed\n");
	}

	kfree_null(&sector0);
}

static void msc_on_device_removed(const struct usb_dev *ud)
{
	if (!ud) return;
	dprintf("msc: removed slot=%u\n", ud->slot_id);
}

static struct usb_class_ops msc_ops = {
	.name = "msc",
	.on_device_added = msc_on_device_added,
	.on_device_removed = msc_on_device_removed,
};

void usb_msc_init(void)
{
	usb_core_register_class(USB_CLASS_MSC, &msc_ops);
}
