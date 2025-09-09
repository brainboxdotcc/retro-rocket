#include <kernel.h>
#include "e1000.h"
#include <mmio.h>

static uint8_t bar_type = 0;				// Type of BAR0
static uint16_t io_base = 0;				// IO Base Address
static uint64_t mem_base = 0;				// MMIO Base Address
static bool eerprom_exists = false;			// A flag indicating if eeprom exists
static uint8_t mac[6] = {0};				// A buffer for storing the mack address
static e1000_rx_desc_t *rx_descs[E1000_NUM_RX_DESC];	// Receive Descriptor Buffers
static e1000_tx_desc_t *tx_descs[E1000_NUM_TX_DESC];	// Transmit Descriptor Buffers
static uint16_t rx_cur = 0;				// Current Receive Descriptor Buffer
static uint16_t tx_cur = 0;				// Current Transmit Descriptor Buffer

static void *tx_buffers[E1000_NUM_TX_DESC];

static uint16_t e1000_device_id = 0;

void e1000_write_command(uint16_t p_address, uint32_t p_value) {
	if (bar_type == 0) {
		mmio_write32(mem_base + p_address, p_value);
	} else {
		outl(io_base, p_address);
		outl(io_base + 4, p_value);
	}
}

uint32_t e1000_read_command(uint16_t p_address) {
	if (bar_type == 0) {
		return mmio_read32(mem_base + p_address);
	} else {
		outl(io_base, p_address);
		return inl(io_base + 4);
	}
}

bool e1000_detect_eeprom() {
	uint32_t val = 0;
	e1000_write_command(REG_EEPROM, 0x1);

	for (int i = 0; i < 1000 && !eerprom_exists; i++) {
		val = e1000_read_command(REG_EEPROM);
		// Support both detection modes (original 0x10 and 82541PI's 0x100)
		if ((val & 0x10) || (val & 0x100)) {
			eerprom_exists = true;
		} else {
			eerprom_exists = false;
		}
	}
	return eerprom_exists;
}


uint32_t e1000_read_eeprom(uint8_t addr) {
	uint16_t data = 0;
	uint32_t tmp = 0;
	time_t now = time(NULL);
	if (eerprom_exists) {
		e1000_write_command(REG_EEPROM, (1) | ((uint32_t) (addr) << 8));
		while (now == time(NULL) && !((tmp = e1000_read_command(REG_EEPROM)) & (1 << 4)));
	} else {
		e1000_write_command(REG_EEPROM, (1) | ((uint32_t) (addr) << 2));
		while (now == time(NULL) && !((tmp = e1000_read_command(REG_EEPROM)) & (1 << 1)));
	}
	if (time(NULL) > now) {
		kprintf("e1000: eeprom read timeout on %02x\n", addr);
	}
	data = (uint16_t) ((tmp >> 16) & 0xFFFF);
	return data;
}

bool e1000_read_mac_address() {
	if (eerprom_exists) {
		uint32_t temp;
		temp = e1000_read_eeprom(0);
		mac[0] = temp & 0xff;
		mac[1] = temp >> 8;
		temp = e1000_read_eeprom(1);
		mac[2] = temp & 0xff;
		mac[3] = temp >> 8;
		temp = e1000_read_eeprom(2);
		mac[4] = temp & 0xff;
		mac[5] = temp >> 8;
	} else {
		uint8_t *mem_base_mac_8 = (uint8_t *) (mem_base + 0x5400);
		uint32_t *mem_base_mac_32 = (uint32_t *) (mem_base + 0x5400);
		if (mem_base_mac_32[0] != 0) {
			for (int i = 0; i < 6; i++) {
				mac[i] = mem_base_mac_8[i];
			}
		} else return false;
	}
	return true;
}

void e1000_get_mac_addr(uint8_t *src_mac_addr) {
	memcpy(src_mac_addr, mac, 6);
}

void e1000_receive_init() {

	struct e1000_rx_desc *descs;

	uint8_t *ptr = (uint8_t *) kmalloc_low(sizeof(e1000_rx_desc_t) * E1000_NUM_RX_DESC + 16);
	uintptr_t aligned = ((uintptr_t) ptr + 15) & ~(uintptr_t) 0x0F;
	descs = (struct e1000_rx_desc *) aligned;

	memset(descs, 0, sizeof(e1000_rx_desc_t) * E1000_NUM_RX_DESC);

	for (int i = 0; i < E1000_NUM_RX_DESC; i++) {
		rx_descs[i] = (e1000_rx_desc_t *) ((uint8_t *) descs + i * 16);
		void *raw_buf = kmalloc_low(8192 + 16);
		memset(raw_buf, 0, 8192 + 16);
		uintptr_t aligned_buf = ((uintptr_t) raw_buf + 15) & ~((uintptr_t) 15);
		rx_descs[i]->addr = (uint64_t) aligned_buf;
		rx_descs[i]->status = 0;
	}

	e1000_write_command(REG_RXDESCLO, (uint32_t) (aligned & 0xFFFFFFFF));
	e1000_write_command(REG_RXDESCHI, (uint32_t) (aligned >> 32));

	e1000_write_command(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

	e1000_write_command(REG_RXDESCHEAD, 0);
	e1000_write_command(REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);
	rx_cur = 0;
	e1000_write_command(REG_RCTRL,
			    RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM |
			    RCTL_SECRC | RCTL_BSIZE_8192);

}

void e1000_transmit_init() {
	e1000_tx_desc_t *descs;
	uint8_t *ptr = (uint8_t *) kmalloc_low(sizeof(e1000_tx_desc_t) * E1000_NUM_TX_DESC + 16);
	uintptr_t aligned = ((uintptr_t) ptr + 15) & ~((uintptr_t) 15);
	descs = (e1000_tx_desc_t *) aligned;

	memset(descs, 0, sizeof(e1000_tx_desc_t) * E1000_NUM_TX_DESC);

	for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
		tx_descs[i] = (e1000_tx_desc_t *) ((uint8_t *) descs + i * 16);
		tx_descs[i]->addr = 0;
		tx_descs[i]->cmd = 0;
		tx_descs[i]->status = TSTA_DD;
		tx_descs[i]->length = 0;
		tx_descs[i]->cso = 0;
		tx_descs[i]->css = 0;
		tx_descs[i]->special = 0;
	}

	for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
		uint8_t *raw = (uint8_t *) kmalloc_low(65536 + E1000_TX_ALIGN);
		memset(raw, 0, 65536 + E1000_TX_ALIGN);
		uintptr_t aligned_addr = ((uintptr_t) raw + E1000_TX_ALIGN - 1) & ~(E1000_TX_ALIGN - 1);
		tx_buffers[i] = (void *) aligned_addr;
		tx_descs[i]->addr = (uint64_t) (uintptr_t) tx_buffers[i];
		tx_descs[i]->status = TSTA_DD; /* ring slot free */
	}

	/* Program TX ring base to the ALIGNED address. */
	e1000_write_command(REG_TXDESCLO, (uint32_t) (aligned & 0xFFFFFFFF));
	e1000_write_command(REG_TXDESCHI, (uint32_t) (aligned >> 32));


	//now setup total length of descriptors
	e1000_write_command(REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);


	//setup numbers
	e1000_write_command(REG_TXDESCHEAD, 0);
	e1000_write_command(REG_TXDESCTAIL, E1000_NUM_TX_DESC - 1);
	tx_cur = 0;
	e1000_write_command(REG_TCTRL, TCTL_EN
				       | TCTL_PSP
				       | (15 << TCTL_CT_SHIFT)
				       | (64 << TCTL_COLD_SHIFT)
				       | TCTL_RTLC);

	e1000_write_command(REG_TIPG, 0x0060200A); // Enable inter-packet gaps
}

void e1000_handle_receive() {

	if (!rx_descs[rx_cur]) {
		dprintf("No rx_desc[rx_cur]\n");
		return;
	}
	netdev_t *dev = get_active_network_device();
	if (!dev || dev->deviceid != ((INTEL_VEND << 16) | e1000_device_id)) {
		dprintf("invalid dev in handle receive\n");
		return;
	}

	while (rx_descs[rx_cur]->status & 0x1) {
		uint8_t *buf = (uint8_t *) rx_descs[rx_cur]->addr;
		uint16_t len = rx_descs[rx_cur]->length;
		if (!buf) {
			dprintf("No buf\n");
			return;
		}

		ethernet_handle_packet((ethernet_frame_t *) buf, len);

		rx_descs[rx_cur]->status = 0;
		uint16_t old_cur = rx_cur;
		rx_cur = (rx_cur + 1) % E1000_NUM_RX_DESC;
		e1000_write_command(REG_RXDESCTAIL, old_cur);
	}

	if (get_ticks() > 5000) {
		int *a = 0;
		*a = 0;
	}
}

void e1000_check_link() {
	uint32_t status = e1000_read_command(REG_STATUS);
	dprintf("e1000: link is %s\n", (status & 2) ? "up" : "down");
}

bool e1000_send_packet(void *p_data, uint16_t p_len) {
	if (p_len > E1000_MAX_PKT_SIZE) {
		dprintf("e1000: packet too large\n");
		return false;
	}
	if (!tx_descs[tx_cur] || !tx_buffers[tx_cur]) {
		dprintf("e1000: Bad send buffer\n");
		return false;
	}

	// Check if descriptor is available
	if (!(tx_descs[tx_cur]->status & TX_DD)) {
		dprintf("e1000: TX ring full, dropping packet\n");
		return false;
	}

	// Copy the data into the pre-allocated <4GiB DMA-safe buffer
	memcpy(tx_buffers[tx_cur], p_data, p_len);

	tx_descs[tx_cur]->length = p_len;
	tx_descs[tx_cur]->cso = 0;
	tx_descs[tx_cur]->css = 0;
	tx_descs[tx_cur]->special = 0;
	tx_descs[tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS;
	tx_descs[tx_cur]->status = 0;

	// Advance the tail
	uint16_t old_cur = tx_cur;
	tx_cur = (tx_cur + 1) % E1000_NUM_TX_DESC;
	e1000_write_command(REG_TXDESCTAIL, tx_cur);

	[[maybe_unused]] volatile uint32_t flush = e1000_read_command(REG_STATUS);

	// Wait for descriptor to complete
	time_t now = time(NULL);
	while (!(tx_descs[old_cur]->status & TX_DD)) {
		if (time(NULL) - now > 1) {
			dprintf("e1000: TX timeout\n");
			break;
		}
	}

	return true;
}

void e1000_up() {
}

/**
 * IRQ handler (MSI)
 * @return
 */
void e1000_handler(uint8_t isr, uint64_t error, uint64_t irq, void *opaque) {
	uint32_t status = e1000_read_command(REG_ICR);
	if (status & ICR_RXT0) {
		e1000_handle_receive();
	}
}

void e1000_enable_interrupts() {
	uint32_t imask =
		IMS_TXDW | // Transmit Descriptor Written Back
		IMS_LSC | // Link Status Change
		IMS_RXSEQ | // Receive Sequence Error
		IMS_RXDMT0 | // Receive Descriptor Minimum Threshold
		IMS_RXO | // Receiver Overrun
		IMS_RXT0 | // Receiver Timer
		IMS_MDAC | // MDI/O access completed
		IMS_RXCFG | // RX config interrupt
		IMS_GPI_EN0 | // General-purpose interrupt 0
		IMS_GPI_EN1 | // General-purpose interrupt 1
		IMS_INT_ASSERT | // "Interrupt Asserted" bit
		IMS_THSTAT | // Thermal status
		IMS_TEMP;        // Temperature sensor

	// Do not enable TXQE unless you are *really sure* you want it.
	e1000_write_command(REG_IMASK, imask);

	// Disable bit 1 (TX Queue Empty) - safe default
	e1000_write_command(REG_IMASK, imask & ~IMS_TXQE);

	// Read ICR to acknowledge any pending IRQs
	e1000_read_command(REG_ICR);
}

bool e1000_start(pci_dev_t *pci_device) {

	// Cache actual detected device ID
	e1000_device_id = pci_read(*pci_device, PCI_DEVICE_ID);

	// Reject unsupported devices
	if (e1000_device_id != E1000_82540EM && e1000_device_id != E1000_82541PI) {
		dprintf("e1000: Attempt to start unsupported device ID\n");
		return false;
	}

	if (e1000_detect_eeprom()) {
		if (!e1000_read_mac_address()) {
			dprintf("e1000: Failed to read device MAC\n");
			return false;
		}
	} else {
		uint8_t *mem_base_mac = (uint8_t *) (mem_base + 0x5400);
		for (int i = 0; i < 6; ++i) {
			mac[i] = mem_base_mac[i];
		}
	}

	interrupts_off();
	e1000_up();

	for (int i = 0; i < 0x80; i++) {
		e1000_write_command(0x5200 + i * 4, 0);
	}

	if (e1000_device_id == E1000_82540EM) {
		/* Attempting MSI setup is safe here */
		pci_setup_interrupt("e1000", *pci_device, logical_cpu_id(), e1000_handler, NULL);
	} else {
		/* But not here! The 82541PI actively torpedoes the system if you enable MSI, see its errata:
		 *
		 * 82541PI GIGABIT ETHERNET CONTROLLER SPECIFICATION UPDATE
		 *
		 * 7. Message Signaled Interrupt Feature May Corrupt Write Transactions
		 *
		 * Problem: The problem is with the implementation of the Message Signaled Interrupt (MSI) feature in the Ethernet
		 * controllers. During MSI writes, the controller incorrectly accesses the write data FIFO. If there are pending write
		 * transactions when this occurs, these transactions may become corrupted, which may cause the network
		 * controller to lock up and become unresponsive.
		 *
		 * For a normal PCI write transaction, the controller’s PCI logic receives data to be written from an internal FIFO.
		 * Once the controller is given bus ownership, the PCI logic pulls the data out of this FIFO and performs the write
		 * transaction.
		 *
		 * For systems using MSI writes, the data, which is constant, should be pulled from the controller’s PCI
		 * Configuration Space rather than the internal FIFO. The affected devices are not pulling this data from PCI
		 * Configuration Space. Instead, they are pulling data from the internal FIFO.
		 *
		 * Implication: If the affected products are used with a future OS that uses Message Signal Interrupts and no accommodations
		 * are made to mitigate the use of these interrupts, data integrity issues may occur.
		 *
		 * Workaround: For PCI systems, advertisement of the MSI capability can be turned off by setting the MSI Disable bit in the
		 * EEPROM (Init Control Word 2, bit 7).
		 *
		 * Status: Intel does not plan to resolve this erratum in the 82541 Gigabit Ethernet controller.
		 */
		uint32_t irq_num = pci_read(*pci_device, PCI_INTERRUPT_LINE);
		register_interrupt_handler(IRQ_START + irq_num, e1000_handler, *pci_device, NULL);

	}

	e1000_receive_init();
	e1000_transmit_init();
	e1000_enable_interrupts();

	// Set link up and speed detection enable - required for PI series, non-op on original e1000
	e1000_write_command(REG_CTRL, 0x20 | ECTRL_SLU);

	if (e1000_device_id == E1000_82541PI) {
		sleep(10); // Delay required after CTRL write on 82541PI
	}

	e1000_check_link();

	kprintf("e1000: MAC %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	netdev_t *net = kmalloc(sizeof(netdev_t));
	if (!net) {
		dprintf("e1000: Out of memory\n");
		interrupts_on();
		return false;
	}
	net->opaque = NULL;
	net->deviceid = (INTEL_VEND << 16) | e1000_device_id;
	make_unique_device_name("net", net->name, sizeof(net->name));
	net->description = "Intel e1000 Gigabit";
	net->flags = CONNECTED;
	net->mtu = 0;
	net->netproto = NULL;
	net->num_netprotos = 0;
	net->speed = 1000;
	net->get_mac_addr = e1000_get_mac_addr;
	net->send_packet = e1000_send_packet;
	net->next = NULL;
	register_network_device(net);

	interrupts_on();

	return true;
}

void init_e1000() {
	pci_dev_t pci_device;
	bool found = false;

	// Try supported devices only
	const uint16_t supported[] = {E1000_82540EM, E1000_82541PI};
	for (size_t i = 0; i < sizeof(supported) / sizeof(supported[0]); i++) {
		pci_device = pci_get_device(INTEL_VEND, supported[i], -1);
		if (!pci_not_found(pci_device)) {
			found = true;
			break;
		}
	}
	if (!found) {
		dprintf("e1000: No compatible devices found\n");
		return;
	}
	uint32_t ret = pci_read(pci_device, PCI_BAR0);
	bar_type = pci_bar_type(ret);
	io_base = pci_io_base(ret);
	mem_base = pci_mem_base(ret);

	kprintf("e1000: mmio base %lx\n", mem_base);

	pci_bus_master(pci_device);
	eerprom_exists = false;

	if (e1000_start(&pci_device)) {
		network_setup();
	}
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("e1000: loaded\n");
	init_e1000();
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	return false;
}
