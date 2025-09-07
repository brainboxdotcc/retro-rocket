/**
 * @file e1000.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#pragma once

#include "kernel.h"

#define INTEL_VEND     (uint32_t)0x8086  // Vendor ID for Intel
#define E1000_82540EM  (uint32_t)0x100E  // Device ID for the e1000 Qemu, Bochs, and VirtualBox emulated NICs
#define E1000_I217     (uint32_t)0x153A  // Device ID for Intel I217
#define E1000_82577LM  (uint32_t)0x10EA  // Device ID for Intel 82577LM
#define E1000_82541PI  (uint32_t)0x107C  // Device ID for Intel 82541PI
 
 
#define REG_CTRL        0x0000
#define REG_STATUS      0x0008
#define REG_EEPROM      0x0014
#define REG_CTRL_EXT    0x0018
#define REG_ICR		0x00C0
#define REG_IMASK       0x00D0
#define REG_RCTRL       0x0100
#define REG_RXDESCLO    0x2800
#define REG_RXDESCHI    0x2804
#define REG_RXDESCLEN   0x2808
#define REG_RXDESCHEAD  0x2810
#define REG_RXDESCTAIL  0x2818
 
#define REG_TCTRL       0x0400
#define REG_TXDESCLO    0x3800
#define REG_TXDESCHI    0x3804
#define REG_TXDESCLEN   0x3808
#define REG_TXDESCHEAD  0x3810
#define REG_TXDESCTAIL  0x3818

#define ICR_TXDW	1 << 0)
#define ICR_TXQE	(1 << 1)
#define ICR_LSC		(1 << 2)
#define ICR_RXSEQ	(1 << 3)
#define ICR_RXDMT0	(1 << 4)
#define ICR_RXO		(1 << 6)
#define ICR_RXT0	(1 << 7)

#define REG_RDTR         0x2820 // RX Delay Timer Register
#define REG_RXDCTL       0x3828 // RX Descriptor Control
#define REG_RADV         0x282C // RX Int. Absolute Delay Timer
#define REG_RSRPD        0x2C00 // RX Small Packet Detect Interrupt

#define E1000_MAX_PKT_SIZE 2048
#define E1000_TX_ALIGN 16

#define TX_DD		0x1

#define IMS_TXDW       (1 << 0)    // Transmit Descriptor Written Back
#define IMS_TXQE       (1 << 1)    // Transmit Queue Empty
#define IMS_LSC        (1 << 2)    // Link Status Change
#define IMS_RXSEQ      (1 << 3)    // Receive Sequence Error
#define IMS_RXDMT0     (1 << 4)    // Receive Descriptor Minimum Threshold Reached
#define IMS_RXO        (1 << 6)    // Receiver Overrun
#define IMS_RXT0       (1 << 7)    // Receiver Timer Interrupt
#define IMS_MDAC       (1 << 9)    // MDIO Access Complete
#define IMS_RXCFG      (1 << 10)   // Receive Configuration Interrupt
#define IMS_GPI_EN0    (1 << 20)   // General Purpose Interrupt 0
#define IMS_GPI_EN1    (1 << 21)   // General Purpose Interrupt 1
#define IMS_INT_ASSERT (1 << 22)   // Interrupt Asserted (clears ICR)
#define IMS_THSTAT     (1 << 24)   // Thermal Sensor Event
#define IMS_TEMP       (1 << 25)   // Temperature Sensor Event
 
#define REG_TIPG         0x0410      // Transmit Inter Packet Gap
#define ECTRL_SLU        0x40        //set link up
 
 
#define RCTL_EN                         (1 << 1)    // Receiver Enable
#define RCTL_SBP                        (1 << 2)    // Store Bad Packets
#define RCTL_UPE                        (1 << 3)    // Unicast Promiscuous Enabled
#define RCTL_MPE                        (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LPE                        (1 << 5)    // Long Packet Reception Enable
#define RCTL_LBM_NONE                   (0 << 6)    // No Loopback
#define RCTL_LBM_PHY                    (3 << 6)    // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF                 (0 << 8)    // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER              (1 << 8)    // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH               (2 << 8)    // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36                      (0 << 12)   // Multicast Offset - bits 47:36
#define RCTL_MO_35                      (1 << 12)   // Multicast Offset - bits 46:35
#define RCTL_MO_34                      (2 << 12)   // Multicast Offset - bits 45:34
#define RCTL_MO_32                      (3 << 12)   // Multicast Offset - bits 43:32
#define RCTL_BAM                        (1 << 15)   // Broadcast Accept Mode
#define RCTL_VFE                        (1 << 18)   // VLAN Filter Enable
#define RCTL_CFIEN                      (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI                        (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF                        (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF                       (1 << 23)   // Pass MAC Control Frames
#define RCTL_BSEX			(1 << 25)   // Big size extension
#define RCTL_SECRC                      (1 << 26)   // Strip Ethernet CRC
 
// Buffer Sizes
#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | RCTL_BSEX)
#define RCTL_BSIZE_8192                 ((2 << 16) | RCTL_BSEX)
#define RCTL_BSIZE_16384                ((1 << 16) | RCTL_BSEX)
 
 
// Transmit Command
 
#define RXD_STAT_DD       0x01		/* Descriptor Done */
#define RXD_STAT_EOP      0x02		/* End of Packet */

#define CMD_EOP                         (1 << 0)    // End of Packet
#define CMD_IFCS                        (1 << 1)    // Insert FCS
#define CMD_IC                          (1 << 2)    // Insert Checksum
#define CMD_RS                          (1 << 3)    // Report Status
#define CMD_RPS                         (1 << 4)    // Report Packet Sent
#define CMD_VLE                         (1 << 6)    // VLAN Packet Enable
#define CMD_IDE                         (1 << 7)    // Interrupt Delay Enable
 
// TCTL Register
 
#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision
 
#define TSTA_DD                         (1 << 0)    // Descriptor Done
#define TSTA_EC                         (1 << 1)    // Excess Collisions
#define TSTA_LC                         (1 << 2)    // Late Collision
#define LSTA_TU                         (1 << 3)    // Transmit Under-run

#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8
 
typedef struct e1000_rx_desc {
        volatile uint64_t addr;
        volatile uint16_t length;
        volatile uint16_t checksum;
        volatile uint8_t status;
        volatile uint8_t errors;
        volatile uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;
 
typedef struct e1000_tx_desc {
        volatile uint64_t addr;
        volatile uint16_t length;
        volatile uint8_t cso;
        volatile uint8_t cmd;
        volatile uint8_t status;
        volatile uint8_t css;
        volatile uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

void e1000_get_mac_addr(uint8_t* src_mac_addr);

bool e1000_send_packet(void * p_data, uint16_t p_len);

void init_e1000();
