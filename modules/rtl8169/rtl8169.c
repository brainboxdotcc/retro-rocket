// Inspired by the Astral RTL8169 driver: https://github.com/Mathewnd/Astral/blob/rewrite/kernel-src/io/net/rtl8169.c

#include <kernel.h>
#include "rtl8169.h"

static rtl8169_dev_t rtl8169_dev;

static inline void rtl8169_outb(uint32_t reg, uint8_t v)
{
	outb(rtl8169_dev.io_base + reg, v);
}

static inline void rtl8169_outw(uint32_t reg, uint16_t v)
{
	outw(rtl8169_dev.io_base + reg, v);
}

static inline void rtl8169_outl(uint32_t reg, uint32_t v)
{
	outl(rtl8169_dev.io_base + reg, v);
}

static inline uint8_t rtl8169_inb(uint32_t reg)
{
	return inb(rtl8169_dev.io_base + reg);
}

static inline uint16_t rtl8169_inw(uint32_t reg)
{
	return inw(rtl8169_dev.io_base + reg);
}

static inline uint32_t rtl8169_inl(uint32_t reg)
{
	return inl(rtl8169_dev.io_base + reg);
}

static bool rtl8169_phy_read(uint8_t reg, uint16_t* value)
{
	rtl8169_outl(RTL8169_REG_PHYAR, (uint32_t)reg << 16);

	for (int i = 0; i < 30; i++) {
		uint32_t v = rtl8169_inl(RTL8169_REG_PHYAR);
		if (v & RTL8169_PHYAR_BUSY) {
			*value = (uint16_t)(v & 0xffff);
			return true;
		}

		io_wait();
	}

	return false;
}

static bool rtl8169_phy_write(uint8_t reg, uint16_t value)
{
	rtl8169_outl(RTL8169_REG_PHYAR, (uint32_t)value | ((uint32_t)reg << 16) | RTL8169_PHYAR_BUSY);

	for (int i = 0; i < 30; i++) {
		uint32_t v = rtl8169_inl(RTL8169_REG_PHYAR);
		if ((v & RTL8169_PHYAR_BUSY) == 0) {
			return true;
		}

		io_wait();
	}

	return false;
}

static void rtl8169_read_mac(void)
{
	uint32_t word = rtl8169_inl(RTL8169_REG_MAC0);
	rtl8169_dev.mac[0] = (uint8_t)(word & 0xff);
	rtl8169_dev.mac[1] = (uint8_t)((word >> 8) & 0xff);
	rtl8169_dev.mac[2] = (uint8_t)((word >> 16) & 0xff);
	rtl8169_dev.mac[3] = (uint8_t)((word >> 24) & 0xff);

	word = rtl8169_inl(RTL8169_REG_MAC4);
	rtl8169_dev.mac[4] = (uint8_t)(word & 0xff);
	rtl8169_dev.mac[5] = (uint8_t)((word >> 8) & 0xff);
}

static void rtl8169_get_mac_addr(uint8_t* dst)
{
	memcpy(dst, rtl8169_dev.mac, 6);
}

static bool rtl8169_reset(void)
{
	rtl8169_outw(RTL8169_REG_CCR, 0);
	rtl8169_outb(RTL8169_REG_COMMAND, RTL8169_COMMAND_RESET);

	for (int i = 0; i < 50; i++) {
		if ((rtl8169_inb(RTL8169_REG_COMMAND) & RTL8169_COMMAND_RESET) == 0) {
			return true;
		}

		io_wait();
	}

	return false;
}

static void rtl8169_tx_reclaim(void)
{
	while (rtl8169_dev.tx_free < RTL8169_TX_DESCRIPTOR_COUNT) {
		rtl8169_descriptor_t* d = &rtl8169_dev.tx_ring[rtl8169_dev.tx_clean];

		if (d->flags & RTL8169_DESCRIPTOR_OWN) {
			break;
		}

		rtl8169_dev.tx_free++;

		rtl8169_dev.tx_clean++;
		if (rtl8169_dev.tx_clean >= RTL8169_TX_DESCRIPTOR_COUNT) {
			rtl8169_dev.tx_clean = 0;
		}
	}
}

static void rtl8169_rx_drain(void)
{
	while ((rtl8169_dev.rx_ring[rtl8169_dev.rx_next].flags & RTL8169_DESCRIPTOR_OWN) == 0) {
		rtl8169_descriptor_t* d = &rtl8169_dev.rx_ring[rtl8169_dev.rx_next];
		void* buf = rtl8169_dev.rx_bufs[rtl8169_dev.rx_next];

		uint16_t len = d->length;
		if (buf && len >= sizeof(ethernet_frame_t) && len <= RTL8169_RX_BUFFER_SIZE) {
			ethernet_handle_packet((ethernet_frame_t*)buf, len);
		}

		uint16_t eor = d->flags & RTL8169_DESCRIPTOR_EOR;
		d->length = RTL8169_RX_BUFFER_SIZE;
		d->flags = (uint16_t)(eor | RTL8169_DESCRIPTOR_OWN);
		d->vlan = 0;

		rtl8169_dev.rx_next++;
		if (rtl8169_dev.rx_next >= RTL8169_RX_DESCRIPTOR_COUNT) {
			rtl8169_dev.rx_next = 0;
		}
	}
}

static void rtl8169_handler([[maybe_unused]] uint8_t isr, [[maybe_unused]] uint64_t error, [[maybe_unused]] uint64_t irq, [[maybe_unused]] void* opaque)
{
	uint16_t status = rtl8169_inw(RTL8169_REG_IRQ_STATUS);

	while (status) {
		/* Ack first */
		rtl8169_outw(RTL8169_REG_IRQ_STATUS, status);

		if (status & (RTL8169_IRQ_STATUS_TX_OK | RTL8169_IRQ_STATUS_TX_ERROR)) {
			rtl8169_tx_reclaim();
		}

		if (status & (RTL8169_IRQ_STATUS_RX_OK | RTL8169_IRQ_STATUS_RX_ERROR)) {
			rtl8169_rx_drain();
		}

		status = rtl8169_inw(RTL8169_REG_IRQ_STATUS);
	}
}

static bool rtl8169_send_packet(void* data, uint16_t len)
{
	if (!rtl8169_dev.active) {
		return false;
	}

	if (len > 1518) {
		return false;
	}

	if (rtl8169_dev.tx_free == 0) {
		return false;
	}

	uint16_t idx = rtl8169_dev.tx_next;
	rtl8169_descriptor_t* d = &rtl8169_dev.tx_ring[idx];
	void* buf = rtl8169_dev.tx_bufs[idx];

	if (!buf) {
		return false;
	}

	memcpy(buf, data, len);

	uintptr_t phys = (uintptr_t)buf;

	d->addr_low = (uint32_t)(phys & 0xffffffff);
	d->addr_high = (uint32_t)((phys >> 32) & 0xffffffff);
	d->length = len;
	d->vlan = 0;

	d->flags = (uint16_t)((d->flags & RTL8169_DESCRIPTOR_EOR) | RTL8169_DESCRIPTOR_OWN | RTL8169_DESCRIPTOR_FS | RTL8169_DESCRIPTOR_LS);

	rtl8169_dev.tx_free--;

	rtl8169_dev.tx_next++;
	if (rtl8169_dev.tx_next >= RTL8169_TX_DESCRIPTOR_COUNT) {
		rtl8169_dev.tx_next = 0;
	}

	rtl8169_outb(RTL8169_REG_TPP, RTL8169_TPP_NORMAL);

	return true;
}

static void rtl8169_free_rings(void)
{
	if (rtl8169_dev.tx_ring) {
		kfree_aligned(rtl8169_dev.tx_ring);
		rtl8169_dev.tx_ring = NULL;
	}

	if (rtl8169_dev.rx_ring) {
		kfree_aligned(rtl8169_dev.rx_ring);
		rtl8169_dev.rx_ring = NULL;
	}
}

static void rtl8169_free_buffers(void)
{
	for (int i = 0; i < RTL8169_RX_DESCRIPTOR_COUNT; i++) {
		if (rtl8169_dev.rx_bufs[i]) {
			kfree_aligned(rtl8169_dev.rx_bufs[i]);
			rtl8169_dev.rx_bufs[i] = NULL;
		}
	}

	for (int i = 0; i < RTL8169_TX_DESCRIPTOR_COUNT; i++) {
		if (rtl8169_dev.tx_bufs[i]) {
			kfree_aligned(rtl8169_dev.tx_bufs[i]);
			rtl8169_dev.tx_bufs[i] = NULL;
		}
	}
}

static bool rtl8169_setup_rings(void)
{
	size_t tx_bytes = sizeof(rtl8169_descriptor_t) * RTL8169_TX_DESCRIPTOR_COUNT;
	size_t rx_bytes = sizeof(rtl8169_descriptor_t) * RTL8169_RX_DESCRIPTOR_COUNT;

	rtl8169_descriptor_t* tx_ring = kmalloc_aligned(tx_bytes, 256);
	rtl8169_descriptor_t* rx_ring = kmalloc_aligned(rx_bytes, 256);

	if (!tx_ring || !rx_ring) {
		rtl8169_free_rings();
		return false;
	}

	memset(tx_ring, 0, tx_bytes);
	memset(rx_ring, 0, rx_bytes);

	rtl8169_dev.tx_ring = tx_ring;
	rtl8169_dev.rx_ring = rx_ring;

	for (int i = 0; i < RTL8169_RX_DESCRIPTOR_COUNT; i++) {
		void* buf = kmalloc_aligned(RTL8169_RX_BUFFER_SIZE, 16);
		if (!buf) {
			rtl8169_free_rings();
			rtl8169_free_buffers();
			return false;
		}

		uintptr_t addr = (uintptr_t)buf;
		rtl8169_dev.rx_bufs[i] = buf;

		rx_ring[i].addr_low = (uint32_t)(addr & 0xffffffff);
		rx_ring[i].addr_high = (uint32_t)((addr >> 32) & 0xffffffff);
		rx_ring[i].length = RTL8169_RX_BUFFER_SIZE;
		rx_ring[i].flags = RTL8169_DESCRIPTOR_OWN;
		rx_ring[i].vlan = 0;
	}

	for (int i = 0; i < RTL8169_TX_DESCRIPTOR_COUNT; i++) {
		void* buf = kmalloc_aligned(RTL8169_TX_BUFFER_SIZE, 16);
		if (!buf) {
			rtl8169_free_rings();
			rtl8169_free_buffers();
			return false;
		}

		uintptr_t addr = (uintptr_t)buf;
		rtl8169_dev.tx_bufs[i] = buf;

		tx_ring[i].addr_low = (uint32_t)(addr & 0xffffffff);
		tx_ring[i].addr_high = (uint32_t)((addr >> 32) & 0xffffffff);
		tx_ring[i].length = 0;
		tx_ring[i].flags = 0;
		tx_ring[i].vlan = 0;
	}

	tx_ring[RTL8169_TX_DESCRIPTOR_COUNT - 1].flags |= RTL8169_DESCRIPTOR_EOR;
	rx_ring[RTL8169_RX_DESCRIPTOR_COUNT - 1].flags |= RTL8169_DESCRIPTOR_EOR;

	rtl8169_outl(RTL8169_REG_TX_RING_LOW, (uint32_t)((uintptr_t)tx_ring & 0xffffffff));
	rtl8169_outl(RTL8169_REG_TX_RING_HIGH, (uint32_t)(((uintptr_t)tx_ring >> 32) & 0xffffffff));

	rtl8169_outl(RTL8169_REG_RX_RING_LOW, (uint32_t)((uintptr_t)rx_ring & 0xffffffff));
	rtl8169_outl(RTL8169_REG_RX_RING_HIGH, (uint32_t)(((uintptr_t)rx_ring >> 32) & 0xffffffff));

	rtl8169_dev.tx_next = 0;
	rtl8169_dev.tx_clean = 0;
	rtl8169_dev.rx_next = 0;
	rtl8169_dev.tx_free = RTL8169_TX_DESCRIPTOR_COUNT;

	return true;
}

static void rtl8169_phy_bringup(void)
{
	if (!rtl8169_phy_write(RTL8169_PHY_BMCR, RTL8169_PHY_BMCR_RESET)) {
		return;
	}

	for (int i = 0; i < 30; i++) {
		uint16_t bmcr = 0;
		if (!rtl8169_phy_read(RTL8169_PHY_BMCR, &bmcr)) {
			return;
		}

		if ((bmcr & RTL8169_PHY_BMCR_RESET) == 0) {
			break;
		}

		io_wait();
	}

	rtl8169_phy_write(RTL8169_PHY_BMCR, RTL8169_PHY_BMCR_AUTO | RTL8169_PHY_BMCR_RESTART_AUTO);

	uint16_t bmsr = 0;
	rtl8169_phy_read(RTL8169_PHY_BMSR, &bmsr);

	for (int i = 0; i < 50000; i++) {
		if (!rtl8169_phy_read(RTL8169_PHY_BMSR, &bmsr)) {
			return;
		}

		if ((bmsr & (RTL8169_PHY_BMSR_LINK_STATUS | RTL8169_PHY_BMSR_AN_COMPLETE)) == (RTL8169_PHY_BMSR_LINK_STATUS | RTL8169_PHY_BMSR_AN_COMPLETE)) {
			return;
		}

		io_wait();
	}
}

static bool rtl8169_start(pci_dev_t pdev)
{
	memset(&rtl8169_dev, 0, sizeof(rtl8169_dev));

	rtl8169_dev.device_id = (uint16_t)pci_read(pdev, PCI_DEVICE_ID);

	uint32_t bar0 = pci_read(pdev, PCI_BAR0);
	if (pci_bar_type(bar0) != 0) {
		dprintf("rtl8169: BAR0 not IO-mapped\n");
		return false;
	}

	rtl8169_dev.io_base = pci_io_base(bar0);

	pci_enable_iospace(pdev);
	pci_bus_master(pdev);

	if (!rtl8169_reset()) {
		dprintf("rtl8169: reset timeout\n");
		return false;
	}

	rtl8169_outw(RTL8169_REG_IRQ_MASK, 0);
	rtl8169_outw(RTL8169_REG_IRQ_STATUS, 0xffff);

	rtl8169_read_mac();
	kprintf("rtl8169: MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
		rtl8169_dev.mac[0], rtl8169_dev.mac[1], rtl8169_dev.mac[2],
		rtl8169_dev.mac[3], rtl8169_dev.mac[4], rtl8169_dev.mac[5]);

	if (!rtl8169_setup_rings()) {
		dprintf("rtl8169: ring setup failed\n");
		return false;
	}

	rtl8169_outl(RTL8169_REG_RX_CONFIG, RTL8169_RX_CONFIG_ACCEPT_ALL_WITHIN_COMMON_SENSE | RTL8169_RX_CONFIG_DMA_BURST_UNLIMITED | RTL8169_RX_CONFIG_FIFO_THRESHOLD_NONE);

	rtl8169_outb(RTL8169_REG_COMMAND, RTL8169_COMMAND_TX_ENABLE);
	rtl8169_outl(RTL8169_REG_TX_CONFIG, RTL8169_TX_CONFIG_DMA_BURST_UNLIMITED | RTL8169_TX_CONFIG_IFG_NORMAL);

	rtl8169_outw(RTL8169_REG_RX_MAX_SIZE, 1518);
	rtl8169_outb(RTL8169_REG_TX_MAX_SIZE, 0xc);

	pci_setup_interrupt("rtl8169", pdev, logical_cpu_id(), rtl8169_handler, NULL);

	rtl8169_outw(RTL8169_REG_IRQ_MASK, RTL8169_IRQ_MASK_RX_OK | RTL8169_IRQ_MASK_TX_OK | RTL8169_IRQ_MASK_TX_ERROR | RTL8169_IRQ_MASK_RX_ERROR);

	rtl8169_phy_bringup();

	rtl8169_outb(RTL8169_REG_COMMAND, RTL8169_COMMAND_TX_ENABLE | RTL8169_COMMAND_RX_ENABLE);

	make_unique_device_name("net", rtl8169_dev.name, sizeof(rtl8169_dev.name));

	netdev_t* net = kmalloc(sizeof(netdev_t));
	if (!net) {
		return false;
	}

	net->opaque = &rtl8169_dev;
	net->deviceid = (RTL8169_VENDOR_ID << 16) | rtl8169_dev.device_id;
	strlcpy(net->name, rtl8169_dev.name, sizeof(net->name));
	net->description = "Realtek RTL8169/8168 Gigabit";
	net->flags = CONNECTED;
	net->mtu = 1500;
	net->netproto = NULL;
	net->num_netprotos = 0;
	net->speed = 1000;
	net->get_mac_addr = rtl8169_get_mac_addr;
	net->send_packet = rtl8169_send_packet;
	net->next = NULL;

	register_network_device(net);

	rtl8169_dev.active = true;

	return true;
}

static void init_rtl8169(void)
{
	pci_dev_t dev;

	const uint16_t supported[] = {
		RTL8169_DEVICE_ID,
		RTL8168_DEVICE_ID,
	};

	for (size_t i = 0; i < sizeof(supported) / sizeof(supported[0]); i++) {
		dev = pci_get_device(RTL8169_VENDOR_ID, supported[i], -1);

		if (!pci_not_found(dev)) {
			if (rtl8169_start(dev)) {
				network_setup();
			}
			return;
		}
	}
}

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void)
{
	dprintf("rtl8169: loaded\n");
	init_rtl8169();
	return rtl8169_dev.active;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void)
{
	return false;
}
