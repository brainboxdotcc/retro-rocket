#include <kernel.h>

rtl8139_dev_t rtl8139_device;

// Rotating round-robin buffer registers
uint8_t TSAD_array[4] = {0x20, 0x24, 0x28, 0x2C};
uint8_t TSD_array[4] = {0x10, 0x14, 0x18, 0x1C};

// True if the device driver is active
bool active = false;

// True if interrupt activity needs the IRQ status clearing
bool activity = false;

void receive_packet() {
	uint64_t buffer_32 = rtl8139_device.rx_buffer;
	uint16_t* t = (uint16_t*)((uint64_t)buffer_32 + rtl8139_device.current_packet_ptr);
	// Skip packet header, get packet length
	uint16_t packet_length = *(t + 1);

	// Skip, packet header and packet length, now t points to the packet data
	t += 2;

	// Now, ethernet layer starts to handle the packet(be sure to make a copy of the packet, insteading of using the buffer)
	// and probabbly this should be done in a separate thread...
	void * packet = kmalloc(packet_length);
	memcpy(packet, t, packet_length);
	ethernet_handle_packet(packet, packet_length);
	
	kfree(packet);

	rtl8139_device.current_packet_ptr = ((rtl8139_device.current_packet_ptr + packet_length + 4 + 3) & RX_READ_POINTER_MASK) % RX_BUF_SIZE;
	outw(rtl8139_device.io_base + CAPR, rtl8139_device.current_packet_ptr - 0x10);

	activity = true;
}

void rtl8139_handler(uint8_t isr, uint64_t error, uint64_t irq) {
	uint16_t status = inw(rtl8139_device.io_base + 0x3e);
	if(status & TOK) {
		// Sent
	}
	if (status & ROK) {
		// Received
		receive_packet();
	}
	activity = true;
	// OSDev wiki says write 0x05 here, but this is ROK+TOK. We need to CLEAR it to 0 to
	// continue receiving interrupts!
	//outw(rtl8139_device.io_base + 0x3E, 0x05);

}

void rtl8139_timer()
{
	if (active && activity) {
		outw(rtl8139_device.io_base + 0x3E, 0x0);
		activity = false;
	}
}

char* read_mac_addr() {
	uint32_t mac_part1 = inl(rtl8139_device.io_base + 0x00);
	uint16_t mac_part2 = inw(rtl8139_device.io_base + 0x04);
	rtl8139_device.mac_addr[0] = mac_part1 >> 0;
	rtl8139_device.mac_addr[1] = mac_part1 >> 8;
	rtl8139_device.mac_addr[2] = mac_part1 >> 16;
	rtl8139_device.mac_addr[3] = mac_part1 >> 24;

	rtl8139_device.mac_addr[4] = mac_part2 >> 0;
	rtl8139_device.mac_addr[5] = mac_part2 >> 8;

	sprintf(rtl8139_device.mac_addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", rtl8139_device.mac_addr[0], rtl8139_device.mac_addr[1], rtl8139_device.mac_addr[2], rtl8139_device.mac_addr[3], rtl8139_device.mac_addr[4], rtl8139_device.mac_addr[5]);

	return rtl8139_device.mac_addr_str;
}

void get_mac_addr(uint8_t * src_mac_addr) {
	memcpy(src_mac_addr, rtl8139_device.mac_addr, 6);
}

void rtl8139_send_packet(void * data, uint32_t len) {
	if (!active) {
		return;
	}

	// Static buffer below 4GB
	uint32_t transfer_data = rtl8139_device.tx_buffers + 8192 * rtl8139_device.tx_cur;
	void* transfer_data_p = (void*)((uint64_t)rtl8139_device.tx_buffers + 8192 * rtl8139_device.tx_cur);

	memcpy(transfer_data_p, data, len);
	outl(rtl8139_device.io_base + TSAD_array[rtl8139_device.tx_cur], transfer_data);
	outl(rtl8139_device.io_base + TSD_array[rtl8139_device.tx_cur++], len);
	if(rtl8139_device.tx_cur > 3)
		rtl8139_device.tx_cur = 0;
}

bool rtl8139_init() {
	pci_dev_t pci_device = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, -1);
	if (!memcmp(&pci_device, &dev_zero, sizeof(pci_dev_t))) {
		return false;
	}
	uint32_t ret = pci_read(pci_device, PCI_BAR0);
	rtl8139_device.bar_type = ret & 0x1;
	// Get io base or mem base by extracting the high 28/30 bits
	rtl8139_device.io_base = ret & (~0x3);
	rtl8139_device.mem_base = ret & (~0xf);
	rtl8139_device.tx_cur = 0;

	// Enable PCI Bus Mastering
	uint32_t pci_command_reg = pci_read(pci_device, PCI_COMMAND);
	if(!(pci_command_reg & (1 << 2))) {
		pci_command_reg |= (1 << 2);
		pci_write(pci_device, PCI_COMMAND, pci_command_reg);
	}

	// Power on and reset
	outb(rtl8139_device.io_base + 0x52, 0x0);
	outb(rtl8139_device.io_base + 0x37, 0x10);
	while((inb(rtl8139_device.io_base + 0x37) & 0x10) != 0);

	// Allocate receive buffer and send buffers, below 4GB boundary
	rtl8139_device.rx_buffer = kmalloc_low(8192 + 16 + 1500);
	rtl8139_device.tx_buffers = kmalloc_low((8192 + 16 + 1500) * 3);
	memset((void*)(uint64_t)rtl8139_device.rx_buffer, 0x0, 8192 + 16 + 1500);
	outl(rtl8139_device.io_base + 0x30, rtl8139_device.rx_buffer);
	for(int i=0; i < 4; i++) {
		outl(rtl8139_device.io_base + 0x20 + i * 4, rtl8139_device.tx_buffers + i * (8192 + 16 + 1500));
	}

	outw(rtl8139_device.io_base + 0x3C, 0x0005);
	outl(rtl8139_device.io_base + 0x44, 0xf | (1 << 7));
	outb(rtl8139_device.io_base + 0x37, 0x0C);

	rtl8139_device.current_packet_ptr = 0;

	uint32_t irq_num = pci_read(pci_device, PCI_INTERRUPT_LINE);
	register_interrupt_handler(32 + irq_num, rtl8139_handler);

	char* mac_address = read_mac_addr();
	kprintf("RTL8139: MAC=%s\n", mac_address);

	active = true;
	return true;
}
