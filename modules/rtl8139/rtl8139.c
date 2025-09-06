#include <kernel.h>
#include "rtl8139.h"

#define TX_BUF_COUNT  4
#define TX_BUF_SIZE   2048   // More than enough for Ethernet frame
#define ALIGN_256(x)  (((x) + 255) & ~255)
#define ALIGN_16(x)   (((x) + 15) & ~15)

rtl8139_dev_t rtl8139_device;

static bool in_interrupt = false;

void rtl8139_get_mac_addr(uint8_t*);

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

bool packet_is_ok(uint16_t* packet)
{
	uint8_t flags = *((uint8_t*)packet);
	bool bad = (flags & (RX_RUNT | RX_LONG | RX_CRC | RX_FAE));
	if (!bad && (flags & RX_ROK)) {
		if (*(packet + 1) > 8192 || *(packet + 1) < 20) {
			dprintf("Bad packet, out of size range\n");
			return false;
		}
		return true;
	}
	dprintf("Bad packet, RX error flags set: %02x\n", flags);
	return false;
}

void receive_packet() {
	while (true) {

		uint8_t tmp_cmd = rtl_inb(ChipCmd);
		if (tmp_cmd & CR_BUFE) {
			break;
		}

		do {
			uint64_t buffer_32 = rtl8139_device.rx_buffer;
			uint16_t* t = (uint16_t*)((uint64_t)buffer_32 + rtl8139_device.current_packet_ptr);

			if (packet_is_ok(t)) {

				// Skip packet header, get packet length
				uint16_t packet_length = *(t + 1);

				// Skip packet header and packet length, now t points to the packet data
				t += 2;

				if (packet_length) {
					static void* packet = NULL;
					if (packet == NULL){
						packet = kmalloc(packet_length);
						if (packet == NULL) {
							return;
						}
					}
					memcpy(packet, t, packet_length);
					netdev_t* dev = get_active_network_device();
					if (dev && dev->deviceid == ((RTL8139_VENDOR_ID << 16) | RTL8139_DEVICE_ID)) {
						ethernet_handle_packet(packet, packet_length);
					}
					memset(t, 0, packet_length);
				} else {
					dprintf("*** Zero length packet NOT passed to ethernet handler ***\n");
				}

				rtl8139_device.current_packet_ptr = ((rtl8139_device.current_packet_ptr + packet_length + 4 + 3) & RX_READ_POINTER_MASK) % RX_BUF_SIZE;
				rtl_outw(CAPR, rtl8139_device.current_packet_ptr - 0x10);
				rtl_outw(IntrStatus, RX_OK);

			} else {
				dprintf("*** Packet NOT ok, resetting ***\n");
			}

			tmp_cmd = rtl_inb(ChipCmd);

		} while (!(tmp_cmd & CR_BUFE));

	}
}

/*
 * The process of packet receive:
 *
 * 1. Data received from line is stored in the receive FIFO.
 * 2. When Early Receive Threshold is meet, data is moved from FIFO to Receive Buffer.
 * 3. After the whole packet is moved from FIFO to Receive Buffer, the receive packet
 * header(receive status and packet length) is written in front of the packet. CBA is
 * updated to the end of the packet.
 * 4. CMD(BufferEmpty) and ISR(TOK) set.
 * 5. ISR routine called and then driver clear ISR(TOK) and update CAPR.
 */
void rtl8139_handler([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t error, [[maybe_unused]] uint64_t irq, void* opaque) {
	in_interrupt = true;
	uint16_t status = rtl_inw(IntrStatus);

	if (status)
		kprintf("Nonzero rtl status %x\n", status);

	// It is VERY important this write happens BEFORE attempting to receive packets,
	// or interrupts break. The datasheet and online forums/wikis DO NOT (or did not,
	// I fixed this) document this...
	
	if(status & TOK) {
		// Sent
		rtl_outw(IntrStatus, TX_OK);
	}
	if (status & ROK) {
		// Received
		receive_packet();
	}
	in_interrupt = false;
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
	snprintf(rtl8139_device.mac_addr_str, 18, "%02X:%02X:%02X:%02X:%02X:%02X", rtl8139_device.mac_addr[0], rtl8139_device.mac_addr[1], rtl8139_device.mac_addr[2], rtl8139_device.mac_addr[3], rtl8139_device.mac_addr[4], rtl8139_device.mac_addr[5]);
	return rtl8139_device.mac_addr_str;
}

void rtl8139_get_mac_addr(uint8_t* src_mac_addr) {
	memcpy(src_mac_addr, rtl8139_device.mac_addr, 6);
}
/**
 * @brief Send a packet
 * 
 * The process of transmitting a packet:
 * 
 * 1: copy the packet to a physically continuous buffer in memory.
 * 2: Write the descriptor which is functioning
 * (1). Fill in Start Address(physical address) of this buffer.
 * (2). Fill in Transmit Status: the size of this packet, the early transmit threshold, Clear OWN
 * bit in TSD (this starts the PCI operation).
 * 3: As the number of data moved to FIFO meet early transmit threshold, the chip start to move
 * data from FIFO to line..
 * 4: When the whole packet is moved to FIFO, the OWN bit is set to 1.
 * 5: When the whole packet is moved to line, the TOK(in TSD) is set to 1.
 * 6: If TOK(IMR) is set to 1 and TOK(ISR) is set then a interrupt is triggered.
 * 7: Interrupt service routine called, driver should clear TOK(ISR) State Diagram: (TOK,OWN)
 * 
 * @param data 
 * @param len 
 */
bool rtl8139_send_packet(void* data, uint16_t len) {
	if (!rtl8139_device.active) {
		dprintf("rtl8139: send packet on inactive device\n");
		return false;
	}
	
	dprintf("rtl8139_send_packet(%08lx, %d)\n", (uint64_t)data, len);

	interrupts_off();

	// Static buffer below 4GB
	uint32_t transfer_data = rtl8139_device.tx_buffers + 8192 * rtl8139_device.tx_cur;
	void* transfer_data_p = (void*)((uint64_t)rtl8139_device.tx_buffers + 8192 * rtl8139_device.tx_cur);
	
	dprintf("transfer_data_p=%08lx\n", (uint64_t)transfer_data_p);

	// 1: copy the packet to a physically continuous buffer in memory.
	memcpy(transfer_data_p, data, len);
	int old_cur = rtl8139_device.tx_cur * 4;
	rtl8139_device.tx_cur++;
	rtl8139_device.tx_cur %= 4;

	/*
	* 2: Write the descriptor which is functioning
	* (1). Fill in Start Address(physical address) of this buffer.
	* (2). Fill in Transmit Status: the size of this packet, the early transmit threshold, Clear OWN
	* bit in TSD (this starts the PCI operation).
	*/
	rtl_outl(TxAddr0 + old_cur, transfer_data);
	rtl_outl(TxStatus0 + old_cur, len);

	/*
	 * 4: When the whole packet is moved to FIFO, the OWN bit is set to 1.
	 */
	uint32_t tx_status;
	while ((tx_status = rtl_inl(TxStatus0 + old_cur)) & (TX_OWN | TX_TUN | TX_TOK | TX_OWC | TX_TABT | TX_CRS)) {
		if (tx_status & TX_OWN) {
			//dprintf("DMA transfer of packet completed\n");
		}
		if (tx_status & TX_TUN) {
			dprintf("Transmit buffer overrun!\n");
			break;
		}
		if (tx_status & TX_TOK) {
			/*
			 * When the whole packet is moved to line, the TOK(in TSD) is set to 1. 
			 */
			break;
		}
		if (tx_status & TX_OWC) {
			dprintf("Out of window collision\n");
			break;
		}
		if (tx_status & TX_TABT) {
			dprintf("Transfer abort\n");
			break;
		}
		if (tx_status & TX_CRS) {
			dprintf("Carrier sense error\n");
			break;
		}
	}

	interrupts_on();
	return true;
}

void init_rtl8139() {
	pci_dev_t pci_device = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, -1);
	if (pci_not_found(pci_device)) {
		return;
	}

	uint32_t ret = pci_read(pci_device, PCI_BAR0);
	rtl8139_device.bar_type = pci_bar_type(ret);
	rtl8139_device.io_base = pci_io_base(ret);
	rtl8139_device.mem_base = pci_mem_base(pci_read(pci_device, PCI_BAR1));
	rtl8139_device.tx_cur = 0;

	if (rtl8139_device.bar_type == 0) {
		kprintf("rtl8139: BAR0 not IO-mapped! Value: 0x%x\n", rtl8139_device.bar_type);
	}

	interrupts_off();

	// Power on and reset
	rtl_outb(Config1, 0x0);
	rtl_outb(ChipCmd, CMDRESET);
	time_t reset_start = time(NULL);
	while ((rtl_inb(ChipCmd) & CMDRESET) != 0) {
		if (time(NULL) - reset_start >= 3) {
			kprintf("RTL8139: Device would not reset within 3 seconds. Faulty hardware? Not enabled.\n");
			interrupts_on();
			return;
		}
	}

	// Enable PCI Bus Mastering
	pci_bus_master(pci_device);

	// Allocate receive buffer and send buffers, below 4GB boundary
	uint8_t* tx_alloc = (uint8_t*)kmalloc_low((TX_BUF_SIZE + 16) * TX_BUF_COUNT * 2);
	rtl8139_device.tx_buffers = (uint32_t)(void*)ALIGN_16((uintptr_t)tx_alloc);

	uint32_t raw = kmalloc_low(16384 + 256);
	rtl8139_device.rx_buffer = (raw + 255) & ~255; // Align to 256-byte boundary

	rtl_outl(RxBuf, rtl8139_device.rx_buffer);

	rtl_outb(RxEarlyThresh, 0x3C);

	for(int i=0; i < 4; i++) {
		rtl_outl(TxAddr0 + i * 4, rtl8139_device.tx_buffers + i * (8192 + 16 + 2048));
	}

	//rtl_outw(IntrMask, INT_DEFAULT);

	rtl_outl(RxConfig, RX_ACCEPTALLPHYS | RX_ACCEPTMYPHYS | RX_ACCEPTMULTICAST | RX_ACCEPTBROADCAST | RX_CFGWRAP);
	rtl_outb(ChipCmd, CMDRXENB | CMDTXENB);
	rtl_outw(IntrMask, INT_DEFAULT);
	rtl_outw(IntrStatus, 0xFFFF);

	volatile uint8_t dummy = rtl_inb(ChipCmd);
	(void)dummy;

	rtl8139_device.current_packet_ptr = 0;

	uint32_t irq_num = pci_read(pci_device, PCI_INTERRUPT_LINE);
	uint32_t irq_pin = pci_read(pci_device, PCI_INTERRUPT_PIN);
	rtl8139_device.irq = irq_num;
	register_interrupt_handler(IRQ_START + irq_num, rtl8139_handler, pci_device, &rtl8139_device);

	char* mac_address = read_mac_addr();
	kprintf("RTL8139: MAC=%s IO=%04x MMIO=%08x IRQ=%d (PIN#%c)\n", mac_address, rtl8139_device.io_base, rtl8139_device.mem_base, irq_num, irq_pin + 'A' - 1);

	rtl8139_device.active = true;

	make_unique_device_name("net", rtl8139_device.name, sizeof(rtl8139_device.name));

	netdev_t* net = kmalloc(sizeof(netdev_t));
	if (!net) {
		return;
	}
	net->opaque = &rtl8139_device;
	net->deviceid = (RTL8139_VENDOR_ID << 16) | RTL8139_DEVICE_ID;
	strlcpy(net->name, rtl8139_device.name, 16);
	net->description = "RTL8139 10/100";
	net->flags = CONNECTED;
	net->mtu = 0;
	net->netproto = NULL;
	net->num_netprotos = 0;
	net->speed = 100;
	net->get_mac_addr = rtl8139_get_mac_addr;
	net->send_packet = rtl8139_send_packet;
	net->next = NULL;
	register_network_device(net);

	interrupts_on();

	dhcp_discover();
}


bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("rtl8139: loaded\n");
	init_rtl8139();
	return true;
}

void EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	dprintf("rtl8139: unloaded\n");
}
