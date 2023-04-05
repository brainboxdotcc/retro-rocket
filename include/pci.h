#pragma once
#include <kernel.h>

typedef struct {
       uint8_t id;
       uint8_t subclass;
       uint8_t progif;
       const char* description;
} pci_subclass;

typedef union pci_dev {
    uint32_t bits;
    struct {
        uint32_t always_zero    : 2;
        uint32_t field_num      : 6;
        uint32_t function_num   : 3;
        uint32_t device_num     : 5;
        uint32_t bus_num        : 8;
        uint32_t reserved       : 7;
        uint32_t enable         : 1;
    };
} pci_dev_t;

extern pci_dev_t dev_zero;

#define PCI_CONFIG_ADDRESS       0xCF8
#define PCI_CONFIG_DATA          0xCFC

#define PCI_VENDOR_ID            0x00
#define PCI_DEVICE_ID            0x02
#define PCI_COMMAND              0x04
#define PCI_STATUS               0x06
#define PCI_REVISION_ID          0x08
#define PCI_PROG_IF              0x09
#define PCI_SUBCLASS             0x0a
#define PCI_CLASS                0x0b
#define PCI_CACHE_LINE_SIZE      0x0c
#define PCI_LATENCY_TIMER        0x0d
#define PCI_HEADER_TYPE          0x0e
#define PCI_BIST                 0x0f
#define PCI_BAR0                 0x10
#define PCI_BAR1                 0x14
#define PCI_BAR2                 0x18
#define PCI_BAR3                 0x1C
#define PCI_BAR4                 0x20
#define PCI_BAR5                 0x24
#define PCI_CAPABILITIES         0x34
#define PCI_INTERRUPT_LINE       0x3C
#define PCI_INTERRUPT_PIN        0x3D
#define PCI_MIN_GNT              0x3E
#define PCI_MAX_LAT              0x3F
#define PCI_SECONDARY_BUS        0x09

#define PCI_STATUS_INTERRUPT			(1 << 3)
#define PCI_STATUS_CAPABAILITIES_LIST		(1 << 4)
#define PCI_STATUS_66MHZ_CAPABLE		(1 << 5)
#define PCI_STATUS_FAST_BACK_TO_BACK_CAPABLE	(1 << 7)
#define PCI_STATUS_MASTER_DATA_PARITY_ERROR	(1 << 8)
#define PCI_STATUS_DEVSEL_TIMING		(1 << 9) & (1 << 10)
#define PCI_STATUS_SIGNALLED_TARGET_ABORT	(1 << 11)
#define PCI_STATUS_RECEIVED_TARGET_ABORT	(1 << 12)
#define PCI_STATUS_RECEIVED_MASTER_ABORT	(1 << 13)
#define PCI_STATUS_SIGNALLED_SYSTEM_ERROR	(1 << 14)
#define PCI_STATUS_DETECTED_PARITY_ERROR	(1 << 15)

#define PCI_COMMAND_IOSPACE			(1 << 0)
#define PCI_COMMAND_MEMSPACE			(1 << 1)
#define PCI_COMMAND_BUS_MASTER			(1 << 2)
#define PCI_COMMAND_SPECIAL_CYCLES		(1 << 3)
#define PCI_COMMAND_MEMORY_WRITE_INVALIDATE	(1 << 4)
#define PCI_COMMAND_VGA_PALLETE_SNOOP		(1 << 5)
#define PCI_COMMAND_PARITY_ERROR_RESPONSE	(1 << 6)
#define PCI_COMMAND_RESERVED_0			(1 << 7)
#define PCI_COMMAND_SERR_ENABLE			(1 << 8)
#define PCI_COMMAND_FAST_BACK_TO_BACK_ENABLE	(1 << 9)
#define PCI_COMMAND_INTERRUPT_DISABLE		(1 << 10)
#define PCI_COMMAND_RESERVED_1			(1 << 11)

#define PCI_CAPABILITY_MSI	 0x05
#define PCI_MSI_64BIT		 (1 << 7)
#define PCI_MSI_DEASSERT	 (1 << 14)
#define PCI_MSI_EDGETRIGGER	 (1 << 15)
#define PCI_MSI_ENABLE		 (1 << 16)

#define PCI_BAR_TYPE_MEMORY      0x00
#define PCI_BAR_TYPE_IOPORT      0x01

#define PCI_HEADER_TYPE_DEVICE  0
#define PCI_HEADER_TYPE_BRIDGE  1
#define PCI_HEADER_TYPE_CARDBUS 2
#define PCI_TYPE_BRIDGE 0x0604
#define PCI_TYPE_SATA   0x0106
#define PCI_NONE 0xFFFF


#define DEVICE_PER_BUS           32
#define FUNCTION_PER_DEVICE      32

uint32_t pci_read(pci_dev_t dev, uint32_t field);
void pci_write(pci_dev_t dev, uint32_t field, uint32_t value);
uint32_t get_device_type(pci_dev_t dev);
uint32_t get_secondary_bus(pci_dev_t dev);
uint32_t pci_reach_end(pci_dev_t dev);
pci_dev_t pci_scan_function(uint16_t vendor_id, uint16_t device_id, uint32_t bus, uint32_t device, uint32_t function, int device_type);
pci_dev_t pci_scan_device(uint16_t vendor_id, uint16_t device_id, uint32_t bus, uint32_t device, int device_type);
pci_dev_t pci_scan_bus(uint16_t vendor_id, uint16_t device_id, uint32_t bus, int device_type);
pci_dev_t pci_get_device(uint16_t vendor_id, uint16_t device_id, int device_type);
void init_pci();
bool pci_bus_master(pci_dev_t device);
uint8_t pci_bar_type(uint32_t field);
uint16_t pci_io_base(uint32_t field);
uint32_t pci_mem_base(uint32_t field);
bool pci_not_found(pci_dev_t device);

bool pci_enable_msi(pci_dev_t device, uint32_t vector, bool edgetrigger, bool deassert);
void pci_display_device_list();
size_t pci_get_device_list(pci_dev_t** list);
void pci_interrupt_enable(pci_dev_t device, bool enable);