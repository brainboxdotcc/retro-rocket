/**
 * @file rtl8169.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 * @ref Inspired by the Astral RTL8169 driver: https://github.com/Mathewnd/Astral/blob/rewrite/kernel-src/io/net/rtl8169.c
 */
#pragma once

#include <kernel.h>

#define RTL8169_VENDOR_ID 0x10ec
#define RTL8168_DEVICE_ID 0x8168
#define RTL8169_DEVICE_ID 0x8169

#define RTL8169_REG_MAC0 0x0
#define RTL8169_REG_MAC4 0x4

#define RTL8169_REG_TX_RING_LOW 0x20
#define RTL8169_REG_TX_RING_HIGH 0x24

#define RTL8169_REG_COMMAND 0x37
#define RTL8169_COMMAND_RX_ENABLE 4
#define RTL8169_COMMAND_TX_ENABLE 8
#define RTL8169_COMMAND_RESET 16

#define RTL8169_REG_TPP 0x38
#define RTL8169_TPP_NORMAL (1 << 6)

#define RTL8169_REG_IRQ_MASK 0x3c
#define RTL8169_IRQ_MASK_RX_OK 1
#define RTL8169_IRQ_MASK_RX_ERROR 2
#define RTL8169_IRQ_MASK_TX_OK 4
#define RTL8169_IRQ_MASK_TX_ERROR 8

#define RTL8169_REG_IRQ_STATUS 0x3e
#define RTL8169_IRQ_STATUS_RX_OK 1
#define RTL8169_IRQ_STATUS_RX_ERROR 2
#define RTL8169_IRQ_STATUS_TX_OK 4
#define RTL8169_IRQ_STATUS_TX_ERROR 8

#define RTL8169_REG_TX_CONFIG 0x40
#define RTL8169_TX_CONFIG_DMA_BURST_UNLIMITED (7 << 8)
#define RTL8169_TX_CONFIG_IFG_NORMAL (3 << 24)

#define RTL8169_REG_RX_CONFIG 0x44
#define RTL8169_RX_CONFIG_ACCEPT_ALL_WITHIN_COMMON_SENSE 0xe
#define RTL8169_RX_CONFIG_DMA_BURST_UNLIMITED (7 << 8)
#define RTL8169_RX_CONFIG_FIFO_THRESHOLD_NONE (7 << 13)

#define RTL8169_REG_PHYAR 0x60
#define RTL8169_PHYAR_BUSY (1 << 31)

#define RTL8169_REG_RX_MAX_SIZE 0xda
#define RTL8169_REG_CCR 0xe0
#define RTL8169_REG_RX_RING_LOW 0xe4
#define RTL8169_REG_RX_RING_HIGH 0xe8
#define RTL8169_REG_TX_MAX_SIZE 0xec

#define RTL8169_PHY_BMCR 0
#define RTL8169_PHY_BMCR_RESTART_AUTO (1 << 9)
#define RTL8169_PHY_BMCR_AUTO (1 << 12)
#define RTL8169_PHY_BMCR_RESET (1 << 15)

#define RTL8169_PHY_BMSR 1
#define RTL8169_PHY_BMSR_LINK_STATUS (1 << 2)
#define RTL8169_PHY_BMSR_AN_COMPLETE (1 << 5)

#define RTL8169_RX_DESCRIPTOR_COUNT 64
#define RTL8169_TX_DESCRIPTOR_COUNT 64

#define RTL8169_RX_BUFFER_SIZE 1524
#define RTL8169_TX_BUFFER_SIZE 1524

#define RTL8169_DESCRIPTOR_OWN (1 << 15)
#define RTL8169_DESCRIPTOR_EOR (1 << 14)
#define RTL8169_DESCRIPTOR_FS (1 << 13)
#define RTL8169_DESCRIPTOR_LS (1 << 12)

typedef struct rtl8169_descriptor {
	uint16_t length;
	uint16_t flags;
	uint32_t vlan;
	uint32_t addr_low;
	uint32_t addr_high;
} __attribute__((packed)) rtl8169_descriptor_t;

typedef struct rtl8169_dev {
	bool active;

	uint16_t io_base;
	uint16_t vendor_id;
	uint16_t device_id;

	uint8_t mac[6];

	volatile rtl8169_descriptor_t* tx_ring;
	volatile rtl8169_descriptor_t* rx_ring;

	void* tx_bufs[RTL8169_TX_DESCRIPTOR_COUNT];
	void* rx_bufs[RTL8169_RX_DESCRIPTOR_COUNT];

	uint16_t tx_next;
	uint16_t tx_clean;
	uint16_t rx_next;

	uint16_t tx_free;

	char name[16];
} rtl8169_dev_t;
