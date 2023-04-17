/**
 * @file rtl8139.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2023
 */
#pragma once

#include <kernel.h>

// PCI vendor/device ID for RTL8139
#define RTL8139_VENDOR_ID	0x10EC // Realtek
#define RTL8139_DEVICE_ID	0x8139 // 8139

// Receive buffer size
#define RX_BUF_SIZE		8192

#define CAPR			0x38
#define RX_READ_POINTER_MASK	(~3)

enum rtl8139_recv_status {
	ROK			= 0x01,		// Received OK
	RER			= 0x02,		// Receive Error
	TOK	 		= 0x04,		// Transmit OK
	TER			= 0x08,		// Transmit Error
	X_TOK			= 0x8000,
};

enum rtl8139_tx_status_bits {
	TX_SIZE			= 0b00000000000000000001111111111111, // Size of packet mas
	TX_OWN			= 0b00000000000000000010000000000000, // Set to 1 by card when DMA is completed
	TX_TUN			= 0b00000000000000000100000000000000, // Set to 1 by card if transmit buffer overran
	TX_TOK			= 0b00000000000000001000000000000000, // Set to 1 by card if packet was tramitted on wite OK
	TX_EARLY_THRESHOLD	= 0b00000000001111110000000000000000, // TX early threshold level before transmission, 000000 = 8 bytes
	TX_RESERVED		= 0b00000000110000000000000000000000, // Reserved
	TX_NCC			= 0b00001111000000000000000000000000, // Number of collisions count
	TX_CDH			= 0b00010000000000000000000000000000, // Carrier detect heartbeat, 0 in 100mbit mode
	TX_OWC			= 0b00100000000000000000000000000000, // Out of window collision (readonly)
	TX_TABT			= 0b01000000000000000000000000000000, // Transmit aborted (readonly)
	TX_CRS			= 0b10000000000000000000000000000000, // Carrier sense lost (readonly)
};

enum rtl8139_rx_status_bits {
	RX_ROK			= 0b00000000000000000000000000000001, // receive ok
	RX_FAE			= 0b00000000000000000000000000000010, // Frame alignment error
	RX_CRC			= 0b00000000000000000000000000000100, // CRC error
	RX_LONG			= 0b00000000000000000000000000001000, // Long packet received (>4k)
	RX_RUNT			= 0b00000000000000000000000000010000, // Runt packet received (<64 bytes)
	RX_ISE			= 0b00000000000000000000000000100000, // Invalid symbol error
	RX_RESERVED		= 0b00000000000000000000111111000000, // Reserved
	RX_BAR			= 0b00000000000000000001000000000000, // Broadcast address received
	RX_PAM			= 0b00000000000000000010000000000000, // Physical address matched
	RX_MAR			= 0b00000000000000000100000000000000, // Multicast address received
};

enum rtl8139_interrupt_reg_bits {
	RX_OK 			= 0x01,
	RX_ERR 			= 0x02,
	TX_OK 			= 0x04,
	TX_ERR 			= 0x08,
	RX_OVERFLOW 		= 0x10,
	RX_UNDERRUN 		= 0x20,
	RX_FIFO_OVERFLOW 	= 0x40,
	CABLE_LENGTH_CHANGE 	= 0x2000,
	PCS_TIMEOUT 		= 0x4000,
	PCI_ERR 		= 0x8000,
	INT_DEFAULT 		= TX_OK | RX_OK,
};

enum rtl8139_chip_cmd_bits {
	RXBUFEMPTY		= 0x01,
	CMDTXENB		= 0x04,
	CMDRXENB		= 0x08,
	CMDRESET		= 0x10,
};

enum rtl8139_rxconfig_bits {
	RX_ACCEPTALLPHYS	= 0x01,
	RX_ACCEPTMYPHYS		= 0x02,
	RX_ACCEPTMULTICAST	= 0x04,
	RX_ACCEPTBROADCAST	= 0x08,
	RX_ACCEPTRUNT		= 0x10,
	RX_ACCEPTERR		= 0x20,
	RX_CFGWRAP		= 0x80,
};

/**
 * @brief Port registers for RTL8139
 * https://datasheetspdf.com/pdf-file/1092361/RealtekMicroelectronics/RTL8139B/1
 * 5. Register Descriptions
 */
enum rtl8139_registers {
	MAC0			= 0x00, // Ethernet hardware address
	MAC1			= 0x04,
	MAR0			= 0x08, // Multicast filter
	TxStatus0		= 0x10, // Transmit status (Four 32bit registers)
	TxAddr0		  	= 0x20, // Tx descriptors (also four 32bit)
	RxBuf			= 0x30,
	RxEarlyCnt	   	= 0x34,
	RxEarlyStatus		= 0x36,
	ChipCmd			= 0x37,
	RxBufPtr		= 0x38,
	RxBufAddr		= 0x3A,
	IntrMask		= 0x3C,
	IntrStatus	   	= 0x3E,
	TxConfig		= 0x40,
	RxConfig		= 0x44,
	Timer			= 0x48,	// A general-purpose counter
	RxMissed		= 0x4C,	// 24 bits valid, write clears
	Cfg9346		  	= 0x50,
	Config0		  	= 0x51,
	Config1		  	= 0x52,
	FlashReg		= 0x54,
	GPPinData		= 0x58,
	GPPinDir		= 0x59,
	MII_SMI		  	= 0x5A,
	HltClk		   	= 0x5B,
	MultiIntr		= 0x5C,
	TxSummary		= 0x60,
	MII_BMCR		= 0x62,
	MII_BMSR		= 0x64,
	NWayAdvert	   	= 0x66,
	NWayLPAR		= 0x68,
	NWayExpansion		= 0x6A,
	// Undocumented registers, but required for proper operation
	FIFOTMS		  	= 0x70,	// FIFO Control and test
	CSCR			= 0x74,	// Chip Status and Configuration Register
	PARA78		   	= 0x78,
	PARA7c		   	= 0x7C,	// Magic transceiver parameter register
};

#define CR_RST		0x10	// Reset, set to 1 to invoke S/W reset, held to 1 while resetting
#define CR_RE		0x08	// Reciever Enable, enables receiving
#define CR_TE		0x04	// Transmitter Enable, enables transmitting
#define CR_BUFE		0x01	// Rx buffer is empty

/**
 * @brief Transfer description
 */
typedef struct tx_desc {
	uint32_t phys_addr;
	uint32_t packet_size;
} tx_desc_t;

/**
 * @brief PCI device configuration
 */
typedef struct rtl8139_dev {
	bool active;
	char name[16];
	uint8_t bar_type;
	uint16_t io_base;
	uint32_t mem_base;
	int eeprom_exist;
	uint8_t mac_addr[6]; // MAC address in binary form
	char mac_addr_str[19]; // MAC address in string form
	uint32_t rx_buffer;
	uint32_t tx_buffers;
	int tx_cur;
	uint32_t current_packet_ptr;
} rtl8139_dev_t;

/**
 * @brief Send a raw packet
 * 
 * @param data packet data
 * @param len packet length
 */
void rtl8139_send_packet(void* data, uint32_t len);

/**
 * @brief Handle RTL8139 interrupt
 * 
 * @param isr ISR number (32 + ISR)
 * @param error error code
 * @param irq Actual IRQ number
 * @param opaque Pointer to device details
 */
void rtl8139_handler(uint8_t isr, uint64_t error, uint64_t irq, void* opaque);

/**
 * @brief Initialise RTL8139
 * 
 * @return true card detected and initialised
 * @return false card not detected, or would not reset
 */
bool init_rtl8139();

/**
 * @brief Get the MAC address of the card in display format
 * 
 * @return char* MAC address
 */
char* read_mac_addr();

/**
 * @brief Handle receipt of packet from ISR
 */
void receive_packet();

/**
 * @brief Get the mac address from the IO ports
 */
void rtl8139_get_mac_addr();

#define get_mac_addr(x) rtl8139_get_mac_addr(x)
