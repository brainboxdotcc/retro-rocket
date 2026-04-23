/**
 * @file e1000e.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2026
 * @ref Intel 82574L Gigabit Ethernet Controller Datasheet
 * @ref Intel 8257x/8258x Gigabit Ethernet Controller Software Developer’s Manual
 * https://docs.rs-online.com/96e8/0900766b81384733.pdf
 * @ref Linux kernel drivers/net/ethernet/intel/e1000e
 */

#pragma once

#include <kernel.h>

#define INTEL_VEND (uint32_t)0x8086

/*
 * Supported PCIe e1000e family subset.
 * This intentionally targets the common, simpler discrete controllers
 * rather than the later integrated ICH/PCH variants.
 */
#define E1000E_82571EB_COPPER (uint32_t)0x105E
#define E1000E_82572EI (uint32_t)0x10B9
#define E1000E_82572EI_COPPER (uint32_t)0x107D
#define E1000E_82573E (uint32_t)0x108B
#define E1000E_82573L (uint32_t)0x109A
#define E1000E_82574L (uint32_t)0x10D3
#define E1000E_82574LA (uint32_t)0x10F6
#define E1000E_82583V (uint32_t)0x150C
#define E1000E_I217LM (uint32_t)0x153A

/* Common register offsets */
#define REG_CTRL 0x0000
#define REG_STATUS 0x0008
#define REG_EEPROM 0x0014
#define REG_CTRL_EXT 0x0018
#define REG_ICR 0x00C0
#define REG_IMASK 0x00D0
#define REG_IMC 0x00D8

#define REG_RCTRL 0x0100

#define REG_TCTRL 0x0400
#define REG_TIPG 0x0410

#define REG_PBA 0x1000

#define REG_RXDESCLO 0x2800
#define REG_RXDESCHI 0x2804
#define REG_RXDESCLEN 0x2808
#define REG_RXDESCHEAD 0x2810
#define REG_RXDESCTAIL 0x2818
#define REG_RDTR 0x2820
#define REG_RXDCTL 0x2828
#define REG_RADV 0x282C
#define REG_RSRPD 0x2C00

#define REG_TXDESCLO 0x3800
#define REG_TXDESCHI 0x3804
#define REG_TXDESCLEN 0x3808
#define REG_TXDESCHEAD 0x3810
#define REG_TXDESCTAIL 0x3818
#define REG_TIDV 0x3820
#define REG_TXDCTL 0x3828
#define REG_TADV 0x382C

#define REG_RAL0 0x5400
#define REG_RAH0 0x5404
#define REG_MTA 0x5200

/* CTRL bits */
#define E1000_CTRL_FD (1 << 0)
#define E1000_CTRL_ASDE (1 << 5)
#define E1000_CTRL_SLU (1 << 6)
#define E1000_CTRL_ILOS (1 << 7)
#define E1000_CTRL_RST (1 << 26)
#define E1000_CTRL_PHY_RST (1u << 31)

/* CTRL_EXT bits */
#define E1000_CTRL_EXT_EIAME (1 << 24)
#define E1000_CTRL_EXT_IAME (1 << 27)
#define E1000_CTRL_EXT_DRV_LOAD (1 << 28)

/* STATUS bits */
#define E1000_STATUS_FD (1 << 0)
#define E1000_STATUS_LU (1 << 1)
#define E1000_STATUS_TXOFF (1 << 4)
#define E1000_STATUS_SPEED_100 (1 << 6)
#define E1000_STATUS_SPEED_1000 (1 << 7)

/* Interrupt cause / mask bits */
#define ICR_TXDW (1 << 0)
#define ICR_TXQE (1 << 1)
#define ICR_LSC (1 << 2)
#define ICR_RXSEQ (1 << 3)
#define ICR_RXDMT0 (1 << 4)
#define ICR_RXO (1 << 6)
#define ICR_RXT0 (1 << 7)

#define IMS_TXDW (1 << 0)
#define IMS_TXQE (1 << 1)
#define IMS_LSC (1 << 2)
#define IMS_RXSEQ (1 << 3)
#define IMS_RXDMT0 (1 << 4)
#define IMS_RXO (1 << 6)
#define IMS_RXT0 (1 << 7)
#define IMS_MDAC (1 << 9)
#define IMS_RXCFG (1 << 10)
#define IMS_GPI_EN0 (1 << 20)
#define IMS_GPI_EN1 (1 << 21)
#define IMS_INT_ASSERT (1 << 22)
#define IMS_THSTAT (1 << 24)
#define IMS_TEMP (1 << 25)

/* RX control */
#define RCTL_EN (1 << 1)
#define RCTL_SBP (1 << 2)
#define RCTL_UPE (1 << 3)
#define RCTL_MPE (1 << 4)
#define RCTL_LPE (1 << 5)
#define RCTL_LBM_NONE (0 << 6)
#define RCTL_LBM_PHY (3 << 6)
#define RTCL_RDMTS_HALF (0 << 8)
#define RTCL_RDMTS_QUARTER (1 << 8)
#define RTCL_RDMTS_EIGHTH (2 << 8)
#define RCTL_MO_36 (0 << 12)
#define RCTL_MO_35 (1 << 12)
#define RCTL_MO_34 (2 << 12)
#define RCTL_MO_32 (3 << 12)
#define RCTL_BAM (1 << 15)
#define RCTL_VFE (1 << 18)
#define RCTL_CFIEN (1 << 19)
#define RCTL_CFI (1 << 20)
#define RCTL_DPF (1 << 22)
#define RCTL_PMCF (1 << 23)
#define RCTL_BSEX (1 << 25)
#define RCTL_SECRC (1 << 26)

/* RX buffer sizes */
#define RCTL_BSIZE_256 (3 << 16)
#define RCTL_BSIZE_512 (2 << 16)
#define RCTL_BSIZE_1024 (1 << 16)
#define RCTL_BSIZE_2048 (0 << 16)
#define RCTL_BSIZE_4096 ((3 << 16) | RCTL_BSEX)
#define RCTL_BSIZE_8192 ((2 << 16) | RCTL_BSEX)
#define RCTL_BSIZE_16384 ((1 << 16) | RCTL_BSEX)

/* RX descriptor bits */
#define RXD_STAT_DD 0x01
#define RXD_STAT_EOP 0x02

/* TX command bits */
#define CMD_EOP (1 << 0)
#define CMD_IFCS (1 << 1)
#define CMD_IC (1 << 2)
#define CMD_RS (1 << 3)
#define CMD_RPS (1 << 4)
#define CMD_VLE (1 << 6)
#define CMD_IDE (1 << 7)

/* TX control */
#define TCTL_EN (1 << 1)
#define TCTL_PSP (1 << 3)
#define TCTL_CT_SHIFT 4
#define TCTL_COLD_SHIFT 12
#define TCTL_SWXOFF (1 << 22)
#define TCTL_RTLC (1 << 24)

/* TX descriptor status */
#define TSTA_DD (1 << 0)
#define TSTA_EC (1 << 1)
#define TSTA_LC (1 << 2)
#define LSTA_TU (1 << 3)

#define TX_DD 0x1

/* Queue control */
#define E1000_RXDCTL_QUEUE_ENABLE (1 << 25)
#define E1000_TXDCTL_QUEUE_ENABLE (1 << 25)

/* Driver-local policy */
#define E1000E_NUM_RX_DESC 32
#define E1000E_NUM_TX_DESC 8
#define E1000E_MAX_PKT_SIZE 16384
#define E1000E_RX_BUFFER_SIZE 8192

#define E1000E_TX_ALIGN 16
#define E1000E_RX_ALIGN 16

#define E1000E_ICR_LSC (1 << 2)
#define E1000E_ICR_RXDMT0 (1 << 4)
#define E1000E_ICR_RXO (1 << 6)
#define E1000E_ICR_RXT0 (1 << 7)

#define E1000E_TIPG_IPGT 10
#define E1000E_TIPG_IPGR1 8
#define E1000E_TIPG_IPGR2 6
#define E1000E_TIPG_DEFAULT \
	(E1000E_TIPG_IPGT | (E1000E_TIPG_IPGR1 << 10) | (E1000E_TIPG_IPGR2 << 20))

typedef struct e1000e_rx_desc {
	volatile uint64_t addr;
	volatile uint16_t length;
	volatile uint16_t checksum;
	volatile uint8_t status;
	volatile uint8_t errors;
	volatile uint16_t special;
} __attribute__((packed)) e1000e_rx_desc_t;

typedef struct e1000e_tx_desc {
	volatile uint64_t addr;
	volatile uint16_t length;
	volatile uint8_t cso;
	volatile uint8_t cmd;
	volatile uint8_t status;
	volatile uint8_t css;
	volatile uint16_t special;
} __attribute__((packed)) e1000e_tx_desc_t;

void e1000e_get_mac_addr(uint8_t *src_mac_addr);

bool e1000e_send_packet(void *p_data, uint16_t p_len);

void init_e1000e(void);