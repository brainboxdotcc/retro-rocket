#pragma once

#include <kernel.h>
#include <stdint.h>
#include "usb_xhci.h"

#define USB_CLASS_MSC     0x08
#define USB_SUBCLASS_SCSI 0x06
#define USB_PROTO_BULK    0x50

#define CBW_SIG 0x43425355u
#define CSW_SIG 0x53425355u

/**
 * @brief BOT Command Block Wrapper (packed, 31 bytes on wire)
 *
 * This is staged to Bulk-OUT before any data phase.
 */
/* Command Block Wrapper (31 bytes) */
struct msc_cbw {
	uint32_t sig;         /* CBW signature (CBW_SIG) */
	uint32_t tag;         /* echoed back in CSW */
	uint32_t data_len;    /* expected data transfer length */
	uint8_t  flags;       /* bit7=1: IN (device->host), 0: OUT */
	uint8_t  lun;         /* LUN (0..15) */
	union {
		uint8_t  cb_len;  /* length of CBWCB (1..16) */
		uint8_t  cmd_len; /* alias for house style call-sites */
	};
	union {
		uint8_t  cb[16];  /* SCSI CDB bytes */
		uint8_t  cmd[16]; /* alias for house style call-sites */
	};
} __attribute__((packed));

/**
 * @brief BOT Command Status Wrapper (packed, 13 bytes on wire)
 *
 * This is read from Bulk-IN after the data phase.
 */
struct msc_csw {
	uint32_t sig;          /**< Must be MSC_CSW_SIG */
	uint32_t tag;          /**< Echo of CBW tag */
	uint32_t residue;      /**< Host-expected − actually processed */
	uint8_t  status;       /**< 0=Passed, 1=Failed, 2=Phase Error */
} __attribute__((packed));

/**
 * @brief Initialise and register the Mass Storage (BOT) class driver.
 *
 * Safe to call multiple times; subsequent calls are harmless.
 */
void usb_msc_init(void);
