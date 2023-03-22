#include <kernel.h>

rtl8139_dev_t rtl8139_device;

// True if the device driver is active
bool active = false;

// True if interrupt activity needs the IRQ status clearing
bool activity = false;

// IO Helper Functions

static void rtl_outb(uint32_t io, uint8_t v) {
	outb(rtl8139_device.io_base + io, v);
	//*((uint8_t*)((uint64_t)rtl8139_device.io_base + io)) = v;
}

static void rtl_outw(uint32_t io, uint16_t v) {
	outw(rtl8139_device.io_base + io, v);
	//*((uint16_t*)((uint64_t)rtl8139_device.io_base + io)) = v;
}

static void rtl_outl(uint32_t io, uint32_t v) {
	outl(rtl8139_device.io_base + io, v);
	//*((uint32_t*)((uint64_t)rtl8139_device.io_base + io)) = v;
}

static uint8_t rtl_inb(uint32_t io) {
	return inb(rtl8139_device.io_base + io);
	//return *((uint8_t*)((uint64_t)rtl8139_device.io_base + io));
}

static uint16_t rtl_inw(uint32_t io) {
	return inw(rtl8139_device.io_base + io);
	//return *((uint16_t*)((uint64_t)rtl8139_device.io_base + io));
}

static uint32_t rtl_inl(uint32_t io) {
	return inl(rtl8139_device.io_base + io);
	//return *((uint32_t*)((uint64_t)rtl8139_device.io_base + io));
}



void receive_packet() {
	uint64_t buffer_32 = rtl8139_device.rx_buffer;
	uint16_t* t = (uint16_t*)((uint64_t)buffer_32 + rtl8139_device.current_packet_ptr);
	// Skip packet header, get packet length
	uint16_t packet_length = *(t + 1);

	// Skip, packet header and packet length, now t points to the packet data
	t += 2;

	if (packet_length) {
		void* packet = kmalloc(packet_length);
		memcpy(packet, t, packet_length);
		ethernet_handle_packet(packet, packet_length);
		kfree(packet);
		memset(t, 0, packet_length);
	}


	rtl8139_device.current_packet_ptr = ((rtl8139_device.current_packet_ptr + packet_length + 4 + 3) & RX_READ_POINTER_MASK) % RX_BUF_SIZE;

	rtl_outw(CAPR, rtl8139_device.current_packet_ptr - 0x10);

	//rtl8139_device.current_packet_ptr %= RX_BUF_SIZE;
}

void rtl8139_handler(uint8_t isr, uint64_t error, uint64_t irq) {
	uint16_t status = rtl_inw(IntrStatus);

	// It is VERY important this write happens BEFORE attempting to receive packets,
	// or interrupts break. The datasheet and online forums/wikis DO NOT (or did not,
	// i fixed this) document this...
	rtl_outw(IntrStatus, 0x05);
	
	if(status & TOK) {
		// Sent
	}
	if (status & ROK) {
		// Received
		receive_packet();
	}
}

void rtl8139_timer()
{
	/* For packet timeouts, unused at present */
}

char* read_mac_addr() {
	uint32_t mac_part1 = rtl_inl(MAC0);
	uint16_t mac_part2 = rtl_inw(MAC1);
	rtl8139_device.mac_addr[0] = mac_part1 >> 0;
	rtl8139_device.mac_addr[1] = mac_part1 >> 8;
	rtl8139_device.mac_addr[2] = mac_part1 >> 16;
	rtl8139_device.mac_addr[3] = mac_part1 >> 24;

	rtl8139_device.mac_addr[4] = mac_part2 >> 0;
	rtl8139_device.mac_addr[5] = mac_part2 >> 8;

	sprintf(rtl8139_device.mac_addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", rtl8139_device.mac_addr[0], rtl8139_device.mac_addr[1], rtl8139_device.mac_addr[2], rtl8139_device.mac_addr[3], rtl8139_device.mac_addr[4], rtl8139_device.mac_addr[5]);

	return rtl8139_device.mac_addr_str;
}

void rtl8139_get_mac_addr(uint8_t* src_mac_addr) {
	memcpy(src_mac_addr, rtl8139_device.mac_addr, 6);
}

void rtl8139_send_packet(void* data, uint32_t len) {
	if (!active) {
		return;
	}

	// Static buffer below 4GB
	uint32_t transfer_data = rtl8139_device.tx_buffers + 8192 * rtl8139_device.tx_cur;
	void* transfer_data_p = (void*)((uint64_t)rtl8139_device.tx_buffers + 8192 * rtl8139_device.tx_cur);

	memcpy(transfer_data_p, data, len);
	rtl_outl(TxAddr0 + (rtl8139_device.tx_cur * 4), transfer_data);
	rtl_outl(TxStatus0 + (rtl8139_device.tx_cur++ * 4), len);
	rtl8139_device.tx_cur = rtl8139_device.tx_cur % 4;
}

bool rtl8139_init() {
	pci_dev_t pci_device = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, -1);
	if (!memcmp(&pci_device, &dev_zero, sizeof(pci_dev_t))) {
		return false;
	}
	uint32_t ret = pci_read(pci_device, PCI_BAR0);
	rtl8139_device.bar_type = pci_bar_type(ret); //ret & 0x1;
	// Get io base or mem base by extracting the high 28/30 bits
	rtl8139_device.io_base = pci_io_base(ret); //ret & (~0x3);
	rtl8139_device.mem_base = pci_mem_base(pci_read(pci_device, PCI_BAR1)); //ret & (~0xf);
	rtl8139_device.tx_cur = 0;

	// Enable PCI Bus Mastering
	pci_bus_master(pci_device);

	// Power on and reset
	rtl_outb(Config1, 0x0);
	rtl_outb(ChipCmd, 0x10);
	while((rtl_inb(ChipCmd) & 0x10) != 0);

	// Allocate receive buffer and send buffers, below 4GB boundary
	rtl8139_device.rx_buffer = kmalloc_low(8192 + 16 + 1500);
	rtl8139_device.tx_buffers = kmalloc_low((8192 + 16 + 1500) * 3);
	memset((void*)(uint64_t)rtl8139_device.rx_buffer, 0x0, 8192 + 16 + 1500);
	rtl_outl(RxBuf, rtl8139_device.rx_buffer);
	for(int i=0; i < 4; i++) {
		rtl_outl(TxAddr0 + i * 4, rtl8139_device.tx_buffers + i * (8192 + 16 + 1500));
	}

	rtl_outw(IntrMask, 0x0005);
	rtl_outl(RxConfig, 0xf | (1 << 7));
	rtl_outb(ChipCmd, 0x0C);

	rtl8139_device.current_packet_ptr = 0;

	uint32_t irq_num = pci_read(pci_device, PCI_INTERRUPT_LINE);
	register_interrupt_handler(32 + irq_num, rtl8139_handler);

	char* mac_address = read_mac_addr();
	kprintf("RTL8139: MAC=%s IO=%04x MMIO=%08x\n", mac_address, rtl8139_device.io_base, rtl8139_device.mem_base);

	proc_register_idle(rtl8139_timer);

	active = true;
	return true;
}
