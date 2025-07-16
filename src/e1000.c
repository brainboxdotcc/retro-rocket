#include <kernel.h>

uint8_t bar_type;	 // Type of BAR0
uint16_t io_base;	 // IO Base Address
uint64_t  mem_base;   // MMIO Base Address
bool eerprom_exists;  // A flag indicating if eeprom exists
uint8_t mac [6];	  // A buffer for storing the mack address
e1000_rx_desc_t *rx_descs[E1000_NUM_RX_DESC]; // Receive Descriptor Buffers
e1000_tx_desc_t *tx_descs[E1000_NUM_TX_DESC]; // Transmit Descriptor Buffers
uint16_t rx_cur;	  // Current Receive Descriptor Buffer
uint16_t tx_cur;	  // Current Transmit Descriptor Buffer

static void* tx_buffers[E1000_NUM_TX_DESC];

uint8_t mmio_read8(uint64_t p_address)
{
	return *((volatile uint8_t*)(p_address));
}

uint16_t mmio_read16(uint64_t p_address)
{
	return *((volatile uint16_t*)(p_address)); 
}

uint32_t mmio_read32(uint64_t p_address)
{
	return *((volatile uint32_t*)(p_address));
}

uint64_t mmio_read64(uint64_t p_address)
{
	return *((volatile uint64_t*)(p_address));	
}

void mmio_write8(uint64_t p_address,uint8_t p_value)
{
	(*((volatile uint8_t*)(p_address)))=(p_value);
}

void mmio_write16(uint64_t p_address,uint16_t p_value)
{
	(*((volatile uint16_t*)(p_address)))=(p_value);	
}

void mmio_write32(uint64_t p_address,uint32_t p_value)
{
	(*((volatile uint32_t*)(p_address)))=(p_value);
 
}

void mmio_write64(uint64_t p_address,uint64_t p_value)
{
	(*((volatile uint64_t*)(p_address)))=(p_value);	
}

void e1000_write_command(uint16_t p_address, uint32_t p_value)
{
	if (bar_type == 0) {
		mmio_write32(mem_base+p_address,p_value);
	}
	else {
		outl(io_base, p_address);
		outl(io_base + 4, p_value);
	}
}
uint32_t e1000_read_command(uint16_t p_address)
{
	if ( bar_type == 0 )
	{
		return mmio_read32(mem_base+p_address);
	}
	else
	{
		outl(io_base, p_address);
		return inl(io_base + 4);
	}
}

bool e1000_detect_eeprom()
{
	uint32_t val = 0;
	e1000_write_command(REG_EEPROM, 0x1);
 
	for(int i = 0; i < 1000 && ! eerprom_exists; i++)
	{
			val = e1000_read_command(REG_EEPROM);
			if(val & 0x10)
					eerprom_exists = true;
			else
					eerprom_exists = false;
	}
	return eerprom_exists;
}
 
uint32_t e1000_read_eeprom(uint8_t addr)
{
	uint16_t data = 0;
	uint32_t tmp = 0;
		if (eerprom_exists) {
			e1000_write_command(REG_EEPROM, (1) | ((uint32_t) (addr) << 8));
			while( !((tmp = e1000_read_command(REG_EEPROM)) & (1 << 4)) );
		} else {
			e1000_write_command(REG_EEPROM, (1) | ((uint32_t) (addr) << 2));
			while( !((tmp = e1000_read_command(REG_EEPROM)) & (1 << 1)) );
		}
	data = (uint16_t)((tmp >> 16) & 0xFFFF);
	return data;
}

bool e1000_read_mac_address()
{
	if ( eerprom_exists)
	{
		uint32_t temp;
		temp = e1000_read_eeprom(0);
		mac[0] = temp &0xff;
		mac[1] = temp >> 8;
		temp = e1000_read_eeprom(1);
		mac[2] = temp &0xff;
		mac[3] = temp >> 8;
		temp = e1000_read_eeprom(2);
		mac[4] = temp &0xff;
		mac[5] = temp >> 8;
	}
	else
	{
		uint8_t * mem_base_mac_8 = (uint8_t *) (mem_base+0x5400);
		uint32_t * mem_base_mac_32 = (uint32_t *) (mem_base+0x5400);
		if ( mem_base_mac_32[0] != 0 )
		{
			for(int i = 0; i < 6; i++)
			{
				mac[i] = mem_base_mac_8[i];
			}
		}
		else return false;
	}
	return true;
}

void e1000_get_mac_addr(uint8_t* src_mac_addr) {
	memcpy(src_mac_addr, mac, 6);
}

void e1000_receive_init()
{
	uint8_t * ptr;
	struct e1000_rx_desc *descs;
 
	// Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
	// In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones
 
	ptr = (uint8_t *)(kmalloc_low(sizeof(e1000_rx_desc_t)*E1000_NUM_RX_DESC + 16));
 
	descs = (struct e1000_rx_desc *)ptr;
	for(int i = 0; i < E1000_NUM_RX_DESC; i++)
	{
		rx_descs[i] = (e1000_rx_desc_t *)((uint8_t *)descs + i*16);
		rx_descs[i]->addr = (uint64_t)(uint8_t *)(kmalloc_low(8192 + 16));
		rx_descs[i]->status = 0;
	}

	e1000_write_command(REG_TXDESCLO, (uint32_t) ((uint64_t) ptr & 0xFFFFFFFF));
	e1000_write_command(REG_TXDESCHI, (uint32_t) ((uint64_t) ptr >> 32));

	e1000_write_command(REG_RXDESCLO, (uint64_t) ptr);
	e1000_write_command(REG_RXDESCHI, 0);

	e1000_write_command(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

	e1000_write_command(REG_RXDESCHEAD, 0);
	e1000_write_command(REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);
	rx_cur = 0;
	e1000_write_command(REG_RCTRL,
			    RCTL_EN | RCTL_SBP | RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM |
			    RCTL_SECRC | RCTL_BSIZE_8192);
 
}

void e1000_transmit_init()
{
	uint8_t *  ptr;
	e1000_tx_desc_t *descs;
	ptr = (uint8_t *)(kmalloc_low(sizeof(e1000_tx_desc_t)*E1000_NUM_TX_DESC + 16));
 
	descs = (e1000_tx_desc_t *)ptr;
	for (int i = 0; i < E1000_NUM_TX_DESC; i++)
	{
		tx_descs[i] = (e1000_tx_desc_t *)((uint8_t*)descs + i*16);
		tx_descs[i]->addr = 0;
		tx_descs[i]->cmd = 0;
		tx_descs[i]->status = TSTA_DD;
	}

	e1000_write_command(REG_TXDESCHI, (uint32_t) ((uint64_t) ptr >> 32));
	e1000_write_command(REG_TXDESCLO, (uint32_t) ((uint64_t) ptr & 0xFFFFFFFF));
 
 
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

void e1000_handle_receive()
{
	uint16_t old_cur;

	while ((rx_descs[rx_cur]->status & 0x1)) {
		uint8_t *buf = (uint8_t *)rx_descs[rx_cur]->addr;
		uint16_t len = rx_descs[rx_cur]->length;

		ethernet_handle_packet((ethernet_frame_t*)buf, len);
 
		rx_descs[rx_cur]->status = 0;
		old_cur = rx_cur;
		rx_cur = (rx_cur + 1) % E1000_NUM_RX_DESC;
		e1000_write_command(REG_RXDESCTAIL, old_cur);
	}	
}

void e1000_check_link()
{
	uint32_t status = e1000_read_command(REG_STATUS);
	kprintf("e1000: link is %s\n", (status & 2) ? "up" : "down");
}

int e1000_send_packet(const void *p_data, uint16_t p_len)
{
	if (p_len > E1000_MAX_PKT_SIZE) {
		dprintf("e1000: packet too large\n");
		return -1;
	}

	// Check if descriptor is available
	if (!(tx_descs[tx_cur]->status & TX_DD)) {
		dprintf("e1000: TX ring full, dropping packet\n");
		return -1;
	}

	// Copy the data into the preallocated <4GiB DMA-safe buffer
	memcpy(tx_buffers[tx_cur], p_data, p_len);

	// Prepare descriptor
	tx_descs[tx_cur]->length = p_len;
	tx_descs[tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS; // End of packet, insert CRC, report status
	tx_descs[tx_cur]->status = 0;

	// Advance the tail
	uint8_t old_cur = tx_cur;
	tx_cur = (tx_cur + 1) % E1000_NUM_TX_DESC;
	e1000_write_command(REG_TXDESCTAIL, tx_cur);

	volatile uint32_t flush = e1000_read_command(REG_STATUS);

	// Optional: Wait for descriptor to complete
	time_t now = time(NULL);
	while (!(tx_descs[old_cur]->status & TX_DD)) {
		if (time(NULL) - now > 1) {
			dprintf("e1000: TX timeout\n");
			break;
		}
	}

	return 0;
}

void e1000_up()
{
	kprintf("e1000: UP\n");
}

/**
 * IRQ handler (MSI)
 * @return
 */
void e1000_handler([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t error, [[maybe_unused]] uint64_t irq, void* opaque)
{
	uint32_t status = e1000_read_command(REG_ICR);
	dprintf("e1000 int status %x\n", status);
	if (status & ICR_TXQE) {
		dprintf("Transmit queue empty\n");
	}
	if (status & ICR_RXSEQ) {
		dprintf("Recv sequence error\n");
	}
	if (status & ICR_LSC) {
		e1000_up();
	}
	if (status & ICR_RXDMT0) {
		// RX threshold met
	}
	if (status & ICR_RXT0) {
		dprintf("RXT0 fired — packet receive\n");
		e1000_handle_receive();
	}
}

void e1000_enable_interrupts()
{
	uint32_t imask =
		IMS_TXDW       | // Transmit Descriptor Written Back
		IMS_LSC        | // Link Status Change
		IMS_RXSEQ      | // Receive Sequence Error
		IMS_RXDMT0     | // Receive Descriptor Minimum Threshold
		IMS_RXO        | // Receiver Overrun
		IMS_RXT0       | // Receiver Timer
		IMS_MDAC       | // MDI/O access completed
		IMS_RXCFG      | // RX config interrupt
		IMS_GPI_EN0    | // General-purpose interrupt 0
		IMS_GPI_EN1    | // General-purpose interrupt 1
		IMS_INT_ASSERT | // "Interrupt Asserted" bit
		IMS_THSTAT     | // Thermal status
		IMS_TEMP;        // Temperature sensor

	// Do not enable TXQE unless you are *really sure* you want it.
	e1000_write_command(REG_IMASK, imask);

	// Disable bit 1 (TX Queue Empty) — safe default
	e1000_write_command(REG_IMASK, imask & ~IMS_TXQE);

	// Read ICR to acknowledge any pending IRQs
	e1000_read_command(REG_ICR);
}

bool e1000_start (pci_dev_t* pci_device)
{

	e1000_detect_eeprom();
	if (!e1000_read_mac_address()) return false;
	//e1000_printMac();
	e1000_up();
 
	for(int i = 0; i < 0x80; i++)
		e1000_write_command(0x5200 + i * 4, 0);

	uint32_t irq_num = pci_read(*pci_device, PCI_INTERRUPT_LINE);
	register_interrupt_handler(32 + irq_num, e1000_handler, *pci_device, NULL);

	e1000_enable_interrupts();
	e1000_receive_init();
	e1000_transmit_init();

	for (int i = 0; i < E1000_NUM_TX_DESC; i++) {
		// Allocate slightly more than needed
		uint8_t* raw = (uint8_t*)kmalloc_low(E1000_MAX_PKT_SIZE + E1000_TX_ALIGN);
		dprintf("Raw %d: %x\n", i, raw);
		uintptr_t aligned_addr = ((uintptr_t)raw + E1000_TX_ALIGN - 1) & ~(E1000_TX_ALIGN - 1);
		dprintf("Aligned %d: %x\n", i, aligned_addr);
		tx_buffers[i] = (void*)aligned_addr;

		// Set descriptor
		tx_descs[i]->addr = (uint64_t)(uintptr_t)tx_buffers[i];
		tx_descs[i]->status = 1; // Mark available
	}


	//set link up
	e1000_write_command(REG_CTRL, 0x20 | ECTRL_SLU); //set link up, activate auto-speed detection

	e1000_check_link();

	kprintf("e1000: MAC %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	network_up();
	return true;
}

bool init_e1000()
{
	pci_dev_t pci_device = pci_get_device(INTEL_VEND, E1000_DEV, -1);
	if (pci_not_found(pci_device)) {
		return false;
	}
	uint32_t ret = pci_read(pci_device, PCI_BAR0);
	// Get BAR0 type, io_base address and MMIO base address
	bar_type = pci_bar_type(ret);
	io_base = pci_io_base(ret);
	mem_base = pci_mem_base(ret);

	kprintf("e1000: bar %d io base %04x mem base %llx\n", bar_type, io_base, mem_base);
 
	// Enable bus mastering
	pci_bus_master(pci_device);

	eerprom_exists = false;

	return e1000_start(&pci_device);
}