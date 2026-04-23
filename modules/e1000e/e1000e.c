/**
 * @file e1000e.c
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 * @ref Intel 82574L Gigabit Ethernet Controller Datasheet
 * @ref Intel 8257x/8258x Gigabit Ethernet Controller Software Developer’s Manual
 * https://docs.rs-online.com/96e8/0900766b81384733.pdf
 * @ref Linux kernel drivers/net/ethernet/intel/e1000e
 */
#include <kernel.h>
#include "e1000e.h"
#include <mmio.h>

static uint8_t bar_type = 0;
static uint16_t io_base = 0;
static uint64_t mem_base = 0;
static bool eeprom_exists = false;
static uint8_t mac[6] = {0};
static e1000e_rx_desc_t *rx_descs[E1000E_NUM_RX_DESC];
static e1000e_tx_desc_t *tx_descs[E1000E_NUM_TX_DESC];
static uint16_t rx_cur = 0;
static uint16_t tx_cur = 0;
static void *tx_buffers[E1000E_NUM_TX_DESC];
static uint16_t e1000e_device_id = 0;
netdev_t *net = NULL;

static void e1000e_write_command(uint32_t address, uint32_t value) {
	if (bar_type == PCI_BAR_TYPE_MEMORY) {
		mmio_write32(mem_base + address, value);
	} else {
		outl(io_base, address);
		outl(io_base + 4, value);
	}
}

static uint32_t e1000e_read_command(uint32_t address) {
	if (bar_type == PCI_BAR_TYPE_MEMORY) {
		return mmio_read32(mem_base + address);
	}

	outl(io_base, address);
	return inl(io_base + 4);
}

static void e1000e_write_flush(void) {
	volatile uint32_t flush = e1000e_read_command(REG_STATUS);
	(void)flush;
}

static bool e1000e_wait(uint32_t reg, uint32_t mask, bool expect_nonzero, time_t timeout_seconds) {
	time_t start = time(NULL);

	while (((e1000e_read_command(reg) & mask) != 0) != expect_nonzero) {
		if (time(NULL) - start > timeout_seconds) {
			return false;
		}
	}

	return true;
}

static bool e1000e_wait_clear(uint32_t reg, uint32_t mask, time_t timeout_seconds) {
	return e1000e_wait(reg, mask, false, timeout_seconds);
}

static bool e1000e_wait_set(uint32_t reg, uint32_t mask, time_t timeout_seconds) {
	return e1000e_wait(reg, mask, true, timeout_seconds);
}

static bool e1000e_detect_eeprom(void) {
	e1000e_write_command(REG_EEPROM, 1);

	for (int i = 0; i < 1000; i++) {
		uint32_t val = e1000e_read_command(REG_EEPROM);
		if ((val & 0x10) != 0 || (val & 0x100) != 0) {
			eeprom_exists = true;
			return true;
		}
	}

	eeprom_exists = false;
	return false;
}

static uint32_t e1000e_read_eeprom(uint8_t addr) {
	uint32_t tmp = 0;
	uint16_t data = 0;
	time_t start = time(NULL);

	if (eeprom_exists) {
		e1000e_write_command(REG_EEPROM, 1 | ((uint32_t) addr << 8));
		while ((tmp = e1000e_read_command(REG_EEPROM), (tmp & (1 << 4)) == 0)) {
			if (time(NULL) - start > 1) {
				dprintf("e1000e: eeprom read timeout on %02x\n", addr);
				break;
			}
		}
	} else {
		e1000e_write_command(REG_EEPROM, 1 | ((uint32_t) addr << 2));
		while ((tmp = e1000e_read_command(REG_EEPROM), (tmp & (1 << 1)) == 0)) {
			if (time(NULL) - start > 1) {
				dprintf("e1000e: eeprom read timeout on %02x\n", addr);
				break;
			}
		}
	}

	data = (uint16_t) ((tmp >> 16) & 0xFFFF);
	return data;
}

static bool e1000e_read_mac_address(void) {
	uint32_t ral;
	uint32_t rah;

	ral = e1000e_read_command(REG_RAL0);
	rah = e1000e_read_command(REG_RAH0);

	if ((ral != 0 || (rah & 0xFFFF) != 0) && ral != 0xFFFFFFFF) {
		mac[0] = ral & 0xFF;
		mac[1] = (ral >> 8) & 0xFF;
		mac[2] = (ral >> 16) & 0xFF;
		mac[3] = (ral >> 24) & 0xFF;
		mac[4] = rah & 0xFF;
		mac[5] = (rah >> 8) & 0xFF;
		return true;
	}

	if (!e1000e_detect_eeprom()) {
		return false;
	}

	uint32_t temp;

	temp = e1000e_read_eeprom(0);
	mac[0] = temp & 0xFF;
	mac[1] = (temp >> 8) & 0xFF;

	temp = e1000e_read_eeprom(1);
	mac[2] = temp & 0xFF;
	mac[3] = (temp >> 8) & 0xFF;

	temp = e1000e_read_eeprom(2);
	mac[4] = temp & 0xFF;
	mac[5] = (temp >> 8) & 0xFF;

	return true;
}

void e1000e_get_mac_addr(uint8_t *src_mac_addr) {
	memcpy(src_mac_addr, mac, 6);
}

static bool e1000e_reset_hw(void) {
	uint32_t ctrl;
	uint32_t ctrl_ext;

	e1000e_write_command(REG_IMC, 0xFFFFFFFF);
	e1000e_write_command(REG_RCTRL, 0);
	e1000e_write_command(REG_TCTRL, 0);
	e1000e_write_flush();

	/* Disable Master and wait for DMA to stop before reset */
	ctrl = e1000e_read_command(REG_CTRL);
	e1000e_write_command(REG_CTRL, ctrl | E1000_CTRL_GIO_MASTER_DISABLE);
	e1000e_wait_clear(REG_STATUS, E1000_STATUS_GIO_MASTER_ENABLE, 1);

	/* Acquire SWSM Semaphore to prevent Management Engine interference */
	uint32_t swsm = e1000e_read_command(0x05B50);
	e1000e_write_command(0x05B50, swsm | 0x1);

	ctrl = e1000e_read_command(REG_CTRL);
	ctrl |= E1000_CTRL_RST;
	e1000e_write_command(REG_CTRL, ctrl);

	delay_ns(1000000);

	if (!e1000e_wait_clear(REG_CTRL, E1000_CTRL_RST, 1)) {
		dprintf("e1000e: reset timed out\n");
		return false;
	}

	e1000e_write_command(REG_IMC, 0xFFFFFFFF);
	e1000e_read_command(REG_ICR);

	ctrl_ext = e1000e_read_command(REG_CTRL_EXT);
	ctrl_ext |= E1000_CTRL_EXT_DRV_LOAD;
	e1000e_write_command(REG_CTRL_EXT, ctrl_ext);

	/* * MERGE: Scoped I217-LM Hardware Fixes
	 * This must happen AFTER CTRL_RST is cleared but BEFORE
	 * the Transmit/Receive engines are initialized.
	 */
	if (e1000e_device_id == E1000E_I217LM) {
		// Disable DMA Coalescing (offset 0x05B14)
		// This ensures the Head register moves as soon as Tail is updated
		e1000e_write_command(0x05B14, 0);
	}

	uint32_t status = e1000e_read_command(REG_STATUS);
	ctrl = e1000e_read_command(REG_CTRL);
	dprintf("e1000e: reset done. STATUS: %08x, CTRL: %08x\n", status, ctrl);

	return true;
}

static void e1000e_receive_init(void) {

	e1000e_rx_desc_t *descs = kmalloc_aligned(sizeof(e1000e_rx_desc_t) * E1000E_NUM_RX_DESC, E1000E_RX_ALIGN);
	memset(descs, 0, sizeof(e1000e_rx_desc_t) * E1000E_NUM_RX_DESC);

	for (int i = 0; i < E1000E_NUM_RX_DESC; i++) {
		rx_descs[i] = (e1000e_rx_desc_t *) ((uint8_t *) descs + i * sizeof(e1000e_rx_desc_t));
		uint8_t *raw_buf = kmalloc_aligned(E1000E_RX_BUFFER_SIZE, E1000E_RX_ALIGN);
		memset(raw_buf, 0, E1000E_RX_BUFFER_SIZE);
		dprintf("e1000e: rx desc buffer raw[%u]: %p, rx_descs[%u] = %p\n", i, raw_buf, i, rx_descs[i]);
		rx_descs[i]->addr = (uint64_t) raw_buf;
		rx_descs[i]->status = 0;
	}

	dprintf("e1000e: rx desc buffer addr: %p\n", descs);

	e1000e_write_command(REG_RXDESCLO, (uint32_t) ((uintptr_t)descs & 0xFFFFFFFF));
	e1000e_write_command(REG_RXDESCHI, (uint32_t) ((uintptr_t)descs >> 32));
	e1000e_write_command(REG_RXDESCLEN, E1000E_NUM_RX_DESC * sizeof(e1000e_rx_desc_t));
	e1000e_write_command(REG_RXDESCHEAD, 0);
	e1000e_write_command(REG_RXDESCTAIL, E1000E_NUM_RX_DESC - 1);
	e1000e_write_command(REG_RDTR, 0);
	e1000e_write_command(REG_RADV, 0);

	// Scoped I217-LM Packet Latency fix
	// This ensures the RX DMA engine doesn't "stall" waiting for internal FIFO thresholds
	if (e1000e_device_id == E1000E_I217LM) {
		e1000e_write_command(0x02110, 0); // REG_RXCSUM - Disable Checksum Offload initially
	}

	e1000e_write_command(REG_RXDCTL, E1000_RXDCTL_QUEUE_ENABLE);

	rx_cur = 0;

	// MERGE: Ensure RCTL flags are safe for I217-LM
	uint32_t rctl = RCTL_EN | RCTL_SBP | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_2048;

	/* NOTE: On I217-LM, if you use RCTL_LPE (Long Packet Enable) without
	 * proper SWSM handling, RX can hang. Sticking to 2048 is correct.
	 */
	e1000e_write_command(REG_RCTRL, rctl);
	e1000e_write_flush();
}

static void e1000e_transmit_init(void) {
	e1000e_tx_desc_t *descs = kmalloc_aligned(sizeof(e1000e_tx_desc_t) * E1000E_NUM_TX_DESC, E1000E_TX_ALIGN);
	memset(descs, 0, sizeof(e1000e_tx_desc_t) * E1000E_NUM_TX_DESC);

	for (int i = 0; i < E1000E_NUM_TX_DESC; i++) {
		uint8_t *raw;

		tx_descs[i] = (e1000e_tx_desc_t *) ((uint8_t *) descs + i * sizeof(e1000e_tx_desc_t));
		raw = kmalloc_aligned(E1000E_MAX_PKT_SIZE, E1000E_TX_ALIGN);
		memset(raw, 0, E1000E_MAX_PKT_SIZE);
		dprintf("e1000e: tx desc buffer raw[%u]: %p, tx_descs[%u] = %p\n", i, raw, i, tx_descs[i]);
		tx_buffers[i] = raw;
		tx_descs[i]->addr = (uint64_t) raw;
		tx_descs[i]->cmd = 0;
		tx_descs[i]->status = TSTA_DD;
		tx_descs[i]->length = 0;
		tx_descs[i]->cso = 0;
		tx_descs[i]->css = 0;
		tx_descs[i]->special = 0;
	}

	dprintf("e1000e: rx desc buffer addr: %p\n", descs);

	e1000e_write_command(REG_TXDESCLO, (uint32_t) ((uintptr_t)descs & 0xFFFFFFFF));
	e1000e_write_command(REG_TXDESCHI, (uint32_t) ((uintptr_t)descs >> 32));
	e1000e_write_command(REG_TXDESCLEN, E1000E_NUM_TX_DESC * sizeof(e1000e_tx_desc_t));
	e1000e_write_command(REG_TXDESCHEAD, 0);
	e1000e_write_command(REG_TXDESCTAIL, 0);
	e1000e_write_command(REG_TIDV, 0);
	e1000e_write_command(REG_TADV, 0);

	// TIPG must be set before TCTRL is enabled
	e1000e_write_command(REG_TIPG, E1000E_TIPG_DEFAULT);

	tx_cur = 0;

	if (e1000e_device_id == E1000E_I217LM) {
		/*
		 * TXDCTL (0x03828) for I217-LM:
		 * PTHRESH=31, HTHRESH=1, WTHRESH=1
		 * This forces the DMA engine to be extremely aggressive.
		 * Without these, Head often stays 0 because the internal FIFO
		 * is waiting for a 'batch' that never comes.
		 */
		e1000e_write_command(REG_TXDCTL, (31 | (1 << 8) | (1 << 16) | E1000_TXDCTL_QUEUE_ENABLE));
		e1000e_write_command(REG_TCTRL, 0x3003F0FA);
	} else {
		e1000e_write_command(REG_TXDCTL, E1000_TXDCTL_QUEUE_ENABLE);
		e1000e_write_command(
			REG_TCTRL,
			TCTL_EN |
			TCTL_PSP |
			(15 << TCTL_CT_SHIFT) |
			(64 << TCTL_COLD_SHIFT) |
			TCTL_RTLC
		);
	}


	e1000e_write_flush();

	uint32_t tdblo = e1000e_read_command(REG_TXDESCLO);
	uint32_t tlen = e1000e_read_command(REG_TXDESCLEN);
	uint32_t tctl = e1000e_read_command(REG_TCTRL);
	dprintf("e1000e: tx init. TDBLO: %08x, TLEN: %u, TCTL: %08x\n", tdblo, tlen, tctl);
}

static void e1000e_handle_receive(void) {
	netdev_t *dev;

	if (!rx_descs[rx_cur]) {
		dprintf("e1000e: missing rx descriptor\n");
		return;
	}

	dev = get_active_network_device();
	if (!dev || dev->deviceid != ((INTEL_VEND << 16) | e1000e_device_id)) {
		dprintf("e1000e: invalid network device in receive path\n");
		return;
	}

	while ((rx_descs[rx_cur]->status & 0x1) != 0) {
		uint8_t *buf;
		uint16_t len;
		uint16_t old_cur;

		buf = (uint8_t *) (uintptr_t) rx_descs[rx_cur]->addr;
		len = rx_descs[rx_cur]->length;

		if (!buf) {
			dprintf("e1000e: missing rx buffer\n");
			return;
		}

		ethernet_handle_packet((ethernet_frame_t *) buf, len);

		rx_descs[rx_cur]->status = 0;
		old_cur = rx_cur;
		rx_cur = (rx_cur + 1) % E1000E_NUM_RX_DESC;
		e1000e_write_command(REG_RXDESCTAIL, old_cur);
	}
}

static void e1000e_check_link(void) {
	uint32_t status;

	if (!net) {
		return;
	}

	status = e1000e_read_command(REG_STATUS);

	if ((status & E1000_STATUS_LU) == 0) {
		dprintf("e1000e: link is down\n");
		net->flags &= ~CONNECTED;
		return;
	}

	net->flags |= CONNECTED;

	if ((status & E1000_STATUS_SPEED_1000) != 0) {
		net->speed = 1000;
	} else if ((status & E1000_STATUS_SPEED_100) != 0) {
		net->speed = 100;
	} else {
		net->speed = 10;
	}


	dprintf("e1000e: link is up (%u Mbps)\n", net->speed);
}

bool e1000e_send_packet(void *p_data, uint16_t p_len) {
	uint16_t old_cur;

	if (p_len > E1000E_MAX_PKT_SIZE) {
		dprintf("e1000e: packet too large\n");
		return false;
	}

	if (!tx_descs[tx_cur] || !tx_buffers[tx_cur]) {
		dprintf("e1000e: missing tx buffer\n");
		return false;
	}

	if ((tx_descs[tx_cur]->status & TX_DD) == 0) {
		dprintf("e1000e: tx ring full, dropping packet\n");
		return false;
	}

	memcpy(tx_buffers[tx_cur], p_data, p_len);

	tx_descs[tx_cur]->length = p_len;
	tx_descs[tx_cur]->cso = 0;
	tx_descs[tx_cur]->css = 0;
	tx_descs[tx_cur]->special = 0;
	tx_descs[tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS;
	tx_descs[tx_cur]->status = 0;

	__asm__ volatile("sfence" ::: "memory");

	old_cur = tx_cur;
	tx_cur = (tx_cur + 1) % E1000E_NUM_TX_DESC;

	e1000e_write_command(REG_TXDESCTAIL, tx_cur);
	e1000e_write_flush();

	/* * DEBUG: Capture state immediately after the kick.
	 * Check if STATUS bit 19 (GIO Master Enable) is actually set.
	 */
	uint32_t h = e1000e_read_command(REG_TXDESCHEAD);
	uint32_t status = e1000e_read_command(REG_STATUS);
	dprintf("e1000e: kick! TDT:%u TDH:%u STAT:%08x\n", tx_cur, h, status);

	time_t start = time(NULL);
	while ((tx_descs[old_cur]->status & TX_DD) == 0) {
		__asm__ volatile("pause" ::: "memory");
		if (time(NULL) - start > 1) {
			/* * Keep your existing timeout logic!
			 * Add these to your log to see IF the NIC moved:
			 */
			uint32_t head = e1000e_read_command(REG_TXDESCHEAD);
			dprintf("e1000e: tx timeout (Head: %u, Tail: %u)\n", head, tx_cur);
			return false;
		}
	}

	return true;
}

static void e1000e_up(void) {
	uint32_t ctrl;

	ctrl = e1000e_read_command(REG_CTRL);
	ctrl |= E1000_CTRL_ASDE | E1000_CTRL_SLU;
	ctrl &= ~E1000_CTRL_PHY_RST;
	e1000e_write_command(REG_CTRL, ctrl);
	e1000e_write_flush();

	e1000e_wait_set(REG_STATUS, E1000_STATUS_LU, 1);
}

static void e1000e_handler([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t error, [[maybe_unused]] uint64_t irq, [[maybe_unused]] void *opaque) {
	uint32_t status;

	status = e1000e_read_command(REG_ICR);

	if ((status & E1000E_ICR_LSC) != 0) {
		e1000e_check_link();
	}

	if ((status & (E1000E_ICR_RXT0 | E1000E_ICR_RXDMT0 | E1000E_ICR_RXO)) != 0) {
		e1000e_handle_receive();
	}
}

static void e1000e_enable_interrupts(void) {
	uint32_t imask;

	e1000e_write_command(REG_IMC, 0xFFFFFFFF);
	e1000e_read_command(REG_ICR);

	imask = IMS_TXDW |
		IMS_LSC |
		IMS_RXSEQ |
		IMS_RXDMT0 |
		IMS_RXO |
		IMS_RXT0;

	e1000e_write_command(REG_IMASK, imask);
	e1000e_write_flush();
	e1000e_read_command(REG_ICR);
}

static bool e1000e_start(pci_dev_t *pci_device) {
	uint32_t ctrl_ext;

	e1000e_device_id = pci_read(*pci_device, PCI_DEVICE_ID);

	if (!e1000e_reset_hw()) {
		return false;
	}

	e1000e_write_command(REG_PBA, 0x30);

	for (int i = 0; i < 0x80; i++) {
		e1000e_write_command(REG_MTA + i * 4, 0);
	}

	if (!e1000e_read_mac_address()) {
		dprintf("e1000e: failed to read device MAC\n");
		return false;
	}

	dprintf("e1000e: MAC %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	kprintf("e1000e: MAC %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	interrupts_off();

	pci_setup_interrupt("e1000e", *pci_device, logical_cpu_id(), e1000e_handler, NULL);

	e1000e_receive_init();
	e1000e_transmit_init();

	ctrl_ext = e1000e_read_command(REG_CTRL_EXT);
	ctrl_ext |= E1000_CTRL_EXT_DRV_LOAD;
	e1000e_write_command(REG_CTRL_EXT, ctrl_ext);

	e1000e_up();
	e1000e_enable_interrupts();

	net = kmalloc(sizeof(netdev_t));
	if (!net) {
		dprintf("e1000e: out of memory\n");
		interrupts_on();
		return false;
	}

	net->opaque = NULL;
	net->deviceid = (INTEL_VEND << 16) | e1000e_device_id;
	make_unique_device_name("net", net->name, sizeof(net->name));
	net->description = "Intel e1000e Gigabit";
	net->flags = CONNECTED;
	net->mtu = 1500;
	net->speed = 1000;
	net->get_mac_addr = e1000e_get_mac_addr;
	net->send_packet = e1000e_send_packet;
	net->next = NULL;
	register_network_device(net);

	e1000e_check_link();

	interrupts_on();

	return true;
}

void init_e1000e(void) {
	pci_dev_t pci_device;
	bool found = false;
	uint32_t ret;

	const uint16_t supported[] = {
		E1000E_82571EB_COPPER,
		E1000E_82572EI,
		E1000E_82572EI_COPPER,
		E1000E_82573E,
		E1000E_82573L,
		E1000E_82574L,
		E1000E_82574LA,
		E1000E_82583V,
		E1000E_I217LM,
	};

	for (size_t i = 0; i < sizeof(supported) / sizeof(supported[0]); i++) {
		pci_device = pci_get_device(INTEL_VEND, supported[i], -1);
		if (!pci_not_found(pci_device)) {
			found = true;
			break;
		}
	}

	if (!found) {
		dprintf("e1000e: no compatible devices found\n");
		return;
	}

	ret = pci_read(pci_device, PCI_BAR0);
	bar_type = pci_bar_type(ret);
	io_base = pci_io_base(ret);
	mem_base = pci_mem_base(ret);

	if (bar_type == PCI_BAR_TYPE_MEMORY) {
		kprintf("e1000e: mmio base %lx\n", mem_base);
	} else {
		kprintf("e1000e: io base %x\n", io_base);
	}

	pci_bus_master(pci_device);
	eeprom_exists = false;

	if (e1000e_start(&pci_device)) {
		network_setup();
	}
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("e1000e: loaded\n");
	init_e1000e();
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	return false;
}