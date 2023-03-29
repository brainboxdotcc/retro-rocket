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

uint8_t mmio_read8 (uint64_t p_address)
{
	return *((volatile uint8_t*)(p_address));
}

uint16_t mmio_read16 (uint64_t p_address)
{
	return *((volatile uint16_t*)(p_address)); 
}

uint32_t mmio_read32 (uint64_t p_address)
{
	return *((volatile uint32_t*)(p_address));
}

uint64_t mmio_read64 (uint64_t p_address)
{
	return *((volatile uint64_t*)(p_address));	
}

void mmio_write8 (uint64_t p_address,uint8_t p_value)
{
	(*((volatile uint8_t*)(p_address)))=(p_value);
}

void mmio_write16 (uint64_t p_address,uint16_t p_value)
{
	(*((volatile uint16_t*)(p_address)))=(p_value);	
}

void mmio_write32 (uint64_t p_address,uint32_t p_value)
{
	(*((volatile uint32_t*)(p_address)))=(p_value);
 
}

void mmio_write64 (uint64_t p_address,uint64_t p_value)
{
	(*((volatile uint64_t*)(p_address)))=(p_value);	
}

void e1000_writeCommand( uint16_t p_address, uint32_t p_value)
{
	if ( bar_type == 0 )
	{
		mmio_write32(mem_base+p_address,p_value);
	}
	else
	{
		outl(io_base, p_address);
		outl(io_base + 4, p_value);
	}
}
uint32_t e1000_readCommand(uint16_t p_address)
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

bool e1000_detectEEProm()
{
	uint32_t val = 0;
	e1000_writeCommand(REG_EEPROM, 0x1); 
 
	for(int i = 0; i < 1000 && ! eerprom_exists; i++)
	{
			val = e1000_readCommand( REG_EEPROM);
			if(val & 0x10)
					eerprom_exists = true;
			else
					eerprom_exists = false;
	}
	return eerprom_exists;
}
 
uint32_t e1000_eepromRead( uint8_t addr)
{
	uint16_t data = 0;
	uint32_t tmp = 0;
		if ( eerprom_exists)
		{
				e1000_writeCommand( REG_EEPROM, (1) | ((uint32_t)(addr) << 8) );
			while( !((tmp = e1000_readCommand(REG_EEPROM)) & (1 << 4)) );
		}
		else
		{
			e1000_writeCommand( REG_EEPROM, (1) | ((uint32_t)(addr) << 2) );
			while( !((tmp = e1000_readCommand(REG_EEPROM)) & (1 << 1)) );
		}
	data = (uint16_t)((tmp >> 16) & 0xFFFF);
	return data;
}

bool e1000_readMACAddress()
{
	if ( eerprom_exists)
	{
		uint32_t temp;
		temp = e1000_eepromRead( 0);
		mac[0] = temp &0xff;
		mac[1] = temp >> 8;
		temp = e1000_eepromRead( 1);
		mac[2] = temp &0xff;
		mac[3] = temp >> 8;
		temp = e1000_eepromRead( 2);
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

void e1000_rxinit()
{
	uint8_t * ptr;
	struct e1000_rx_desc *descs;
 
	// Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
	// In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones
 
	ptr = (uint8_t *)(kmalloc(sizeof(e1000_rx_desc_t)*E1000_NUM_RX_DESC + 16));
 
	descs = (struct e1000_rx_desc *)ptr;
	for(int i = 0; i < E1000_NUM_RX_DESC; i++)
	{
		rx_descs[i] = (e1000_rx_desc_t *)((uint8_t *)descs + i*16);
		rx_descs[i]->addr = (uint64_t)(uint8_t *)(kmalloc(8192 + 16));
		rx_descs[i]->status = 0;
	}
 
	e1000_writeCommand(REG_TXDESCLO, (uint32_t)((uint64_t)ptr >> 32) );
	e1000_writeCommand(REG_TXDESCHI, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));
 
	e1000_writeCommand(REG_RXDESCLO, (uint64_t)ptr);
	e1000_writeCommand(REG_RXDESCHI, 0);
 
	e1000_writeCommand(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);
 
	e1000_writeCommand(REG_RXDESCHEAD, 0);
	e1000_writeCommand(REG_RXDESCTAIL, E1000_NUM_RX_DESC - 1);
	rx_cur = 0;
	e1000_writeCommand(REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_8192);
 
}

void e1000_txinit()
{
	uint8_t *  ptr;
	e1000_tx_desc_t *descs;
	// Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
	// In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones
	ptr = (uint8_t *)(kmalloc(sizeof(e1000_tx_desc_t)*E1000_NUM_TX_DESC + 16));
 
	descs = (e1000_tx_desc_t *)ptr;
	for(int i = 0; i < E1000_NUM_TX_DESC; i++)
	{
		tx_descs[i] = (e1000_tx_desc_t *)((uint8_t*)descs + i*16);
		tx_descs[i]->addr = 0;
		tx_descs[i]->cmd = 0;
		tx_descs[i]->status = TSTA_DD;
	}
 
	e1000_writeCommand(REG_TXDESCHI, (uint32_t)((uint64_t)ptr >> 32) );
	e1000_writeCommand(REG_TXDESCLO, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));
 
 
	//now setup total length of descriptors
	e1000_writeCommand(REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);
 
 
	//setup numbers
	e1000_writeCommand( REG_TXDESCHEAD, 0);
	e1000_writeCommand( REG_TXDESCTAIL, E1000_NUM_TX_DESC - 1);
	tx_cur = 0;
	e1000_writeCommand(REG_TCTRL,  TCTL_EN
		| TCTL_PSP
		| (15 << TCTL_CT_SHIFT)
		| (64 << TCTL_COLD_SHIFT)
		| TCTL_RTLC);
 
	e1000_writeCommand(REG_TCTRL, (1 << 1) | (1 << 3));
	// This line of code overrides the one before it but I left both to highlight that the previous one works with e1000 cards,
	// but for the e1000e cards 
	// you should set the TCTRL register as follows. For detailed description of each bit, please refer to the Intel Manual.
	// In the case of I217 and 82577LM packets will not be sent if the TCTRL is not configured using the following bits.
	//e1000_writeCommand(REG_TCTRL,  0b0110000000000111111000011111010);
	//e1000_writeCommand(REG_TIPG,  0x0060200a);
 
}

void e1000_handleReceive()
{
	uint16_t old_cur;

	kprintf("Handle recv\n");
  
	while ((rx_descs[rx_cur]->status & 0x1)) {
		uint8_t *buf = (uint8_t *)rx_descs[rx_cur]->addr;
		uint16_t len = rx_descs[rx_cur]->length;

		dump_hex(buf, len);
 
		// Here you should inject the received packet into your network stack
		ethernet_handle_packet((ethernet_frame_t*)buf, len);
 
		rx_descs[rx_cur]->status = 0;
		old_cur = rx_cur;
		rx_cur = (rx_cur + 1) % E1000_NUM_RX_DESC;
		e1000_writeCommand(REG_RXDESCTAIL, old_cur );
	}	
}

void e1000_check_link()
{
	uint32_t status = e1000_readCommand(REG_STATUS);
	kprintf("e1000: link is %s\n", (status & 2) ? "up" : "down");
}

int e1000_send_packet(const void * p_data, uint16_t p_len)
{	
	tx_descs[tx_cur]->addr = (uint64_t)p_data;
	tx_descs[tx_cur]->length = p_len;
	tx_descs[tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS;
	tx_descs[tx_cur]->status = 0;
	uint8_t old_cur = tx_cur;
	tx_cur = (tx_cur + 1) % E1000_NUM_TX_DESC;
	e1000_writeCommand(REG_TXDESCTAIL, tx_cur);
	kprintf("Wrote command\n");   
	time_t now = time(NULL);
	while(!(tx_descs[old_cur]->status & 0x0f)) {
	if (time(NULL) - now > 1) {
		kprintf("Timeout\n");
		break;
	}
	}	
	kprintf("Sent\n");
	return 0;
}

void e1000_up()
{
	kprintf("e1000: UP\n");
}

void e1000_handler(uint8_t isr, uint64_t error, uint64_t irq)
{
	/* This might be needed here if your handler doesn't clear interrupts from each device and must be done before EOI if using the PIC.
	 * Without this, the card will spam interrupts as the int-line will stay high.
	 */
	e1000_writeCommand(REG_IMASK, 0x1);
	//e1000_writeCommand(REG_IMASK ,0x1F6DC);
	//e1000_writeCommand(REG_IMASK ,0xff & ~4);


	uint32_t status = e1000_readCommand(0xc0);
	kprintf("e1000 int status %d\n", status);
	if(status & 0x02)
	{
		kprintf("Transmit queue empty\n");
	}
	if(status & 0x08)
	{
		kprintf("Recv sequence error\n");
	}
	if(status & 0x04)
	{
		e1000_up();
	}
	else if(status & 0x10)
	{
		// good threshold
	}
	else if(status & 0x80)
	{
		kprintf("status 80\n");
		e1000_handleReceive();
	}
}

void e1000_enableInterrupt()
{
	e1000_writeCommand(REG_IMASK ,0x1F6DC);
	e1000_writeCommand(REG_IMASK ,0xff & ~4);
	e1000_readCommand(0xc0);
}

bool e1000_start (pci_dev_t* pci_device)
{
	e1000_detectEEProm ();
	if (! e1000_readMACAddress()) return false;
	//e1000_printMac();
	e1000_up();
 
	for(int i = 0; i < 0x80; i++)
		e1000_writeCommand(0x5200 + i*4, 0);

	uint32_t irq_num = pci_read(*pci_device, PCI_INTERRUPT_LINE);
	register_interrupt_handler(32 + irq_num, e1000_handler);

	e1000_enableInterrupt();
	e1000_rxinit();
	e1000_txinit();

	//set link up
	e1000_writeCommand(REG_CTRL, 0x20 | ECTRL_SLU); //set link up, activate auto-speed detection

	e1000_check_link();

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