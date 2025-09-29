/**
 * @file pci.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
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
#define PCI_CAPABILITY_POINTER   0x34
#define PCI_INTERRUPT_LINE       0x3C
#define PCI_INTERRUPT_PIN        0x3D
#define PCI_MIN_GNT              0x3E
#define PCI_MAX_LAT              0x3F
#define PCI_SECONDARY_BUS        0x09

#define PCI_STATUS_INTERRUPT			(1 << 3)
#define PCI_STATUS_CAPABILITIES_LIST		(1 << 4)
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

#define PCI_CAPABILITY_MSIX	 0x11

#define PCI_MSIX_ENABLE		 (1 << 15)
#define PCI_MSIX_FUNCTION_MASK	 (1 << 14)

#define PCI_BAR_TYPE_MEMORY       0x00
#define PCI_BAR_TYPE_IOPORT       0x01

#define PCI_HEADER_TYPE_DEVICE  0
#define PCI_HEADER_TYPE_BRIDGE  1
#define PCI_HEADER_TYPE_CARDBUS 2
#define PCI_TYPE_BRIDGE 0x0604
#define PCI_TYPE_SATA   0x0106
#define PCI_NONE 0xFFFF


#define DEVICE_PER_BUS           32
#define FUNCTION_PER_DEVICE      32

/**
 * Generic scoring callback: return a signed score; <0 means "reject". Higher wins.
 */
typedef int (*pci_score_fn)(pci_dev_t dev, void *ctx);

/**
 * @brief Read a value from a PCI configuration space field.
 * @param dev PCI device descriptor.
 * @param field Offset of the configuration field.
 * @return The value read (size determined by field).
 */
uint32_t pci_read(pci_dev_t dev, uint32_t field);

/**
 * @brief Write a value into a PCI configuration space field.
 * @param dev PCI device descriptor.
 * @param field Offset of the configuration field.
 * @param value Value to write (size determined by field).
 */
void pci_write(pci_dev_t dev, uint32_t field, uint32_t value);

/**
 * @brief Get the PCI device class/subclass identifier.
 * @param dev PCI device descriptor.
 * @return Encoded device type (class << 8 | subclass).
 */
uint32_t get_device_type(pci_dev_t dev);

/**
 * @brief Get the secondary bus number from a PCI bridge device.
 * @param dev PCI bridge device descriptor.
 * @return Secondary bus number.
 */
uint32_t get_secondary_bus(pci_dev_t dev);

/**
 * @brief Determine if the device is a single-function endpoint.
 * @param dev PCI device descriptor.
 * @return Non-zero if endpoint, zero if multifunction/bridge.
 */
uint32_t pci_reach_end(pci_dev_t dev);

/**
 * @brief Scan a specific PCI function for matching IDs or type.
 * @param vendor_id Vendor ID to match (0 = wildcard).
 * @param device_id Device ID to match (0 = wildcard).
 * @param bus Bus number.
 * @param device Device slot number.
 * @param function Function number.
 * @param device_type Device class/type to match (-1 = any).
 * @return Matching PCI device or dev_zero if none.
 */
pci_dev_t pci_scan_function(uint16_t vendor_id, uint16_t device_id,
			    uint32_t bus, uint32_t device,
			    uint32_t function, int device_type);

/**
 * @brief Scan all functions of a given PCI device slot.
 * @param vendor_id Vendor ID to match (0 = wildcard).
 * @param device_id Device ID to match (0 = wildcard).
 * @param bus Bus number.
 * @param device Device slot number.
 * @param device_type Device class/type to match (-1 = any).
 * @return Matching PCI device or dev_zero if none.
 */
pci_dev_t pci_scan_device(uint16_t vendor_id, uint16_t device_id,
			  uint32_t bus, uint32_t device,
			  int device_type);

/**
 * @brief Scan all devices on a given PCI bus.
 * @param vendor_id Vendor ID to match (0 = wildcard).
 * @param device_id Device ID to match (0 = wildcard).
 * @param bus Bus number.
 * @param device_type Device class/type to match (-1 = any).
 * @return First matching PCI device or dev_zero if none.
 */
pci_dev_t pci_scan_bus(uint16_t vendor_id, uint16_t device_id,
		       uint32_t bus, int device_type);

/**
 * @brief Find a device by vendor, device, or type.
 * @param vendor_id Vendor ID to match (0 = any).
 * @param device_id Device ID to match (0 = any).
 * @param device_type Device class/type to match (-1 = any).
 * @return Matching PCI device or dev_zero if none.
 */
pci_dev_t pci_get_device(uint16_t vendor_id, uint16_t device_id, int device_type);

/**
 * @brief Find the best PCI function matching an optional filter (vendor/device/class),
 * selected by a driver-supplied scoring callback. All params are optional.
 *
 * @param vendor_id Vendor ID to match (0 = any).
 * @param device_id Device ID to match (0 = any).
 * @param device_type Device class/type to match (-1 = any).
 * @param score Scoring function to rank devices
 * @param ctx User defined context to pass to scoring function
 * @return dev_zero if nothing acceptable was found
 */
pci_dev_t pci_get_best(uint16_t vendor_id, uint16_t device_id, int device_type, pci_score_fn score, void *ctx);

pci_dev_t pci_get_device_nth(uint16_t vendor_id, uint16_t device_id, int device_type, size_t nth);

/**
 * @brief Initialise the PCI subsystem and enumerate devices.
 */
void init_pci();

/**
 * @brief Enable bus mastering on a PCI device.
 * @param device PCI device descriptor.
 * @return True if bus mastering was newly enabled, false if already set.
 */
bool pci_bus_master(pci_dev_t device);

/**
 * @brief Get the type of a PCI Base Address Register (BAR).
 * @param field Raw BAR field value.
 * @return 0 = memory, 1 = I/O.
 */
uint8_t pci_bar_type(uint32_t field);

/**
 * @brief Extract an I/O base address from a BAR.
 * @param field Raw BAR field value.
 * @return I/O port base address.
 */
uint16_t pci_io_base(uint32_t field);

/**
 * @brief Extract a memory base address from a BAR.
 * @param field Raw BAR field value.
 * @return Memory-mapped base address.
 */
uint32_t pci_mem_base(uint32_t field);

/**
 * @brief Test whether a device descriptor represents 'not found'.
 * @param device PCI device descriptor.
 * @return True if no device, false otherwise.
 */
bool pci_not_found(pci_dev_t device);

/**
 * @brief Enable MSI (Message Signalled Interrupts) on a device.
 * @param device PCI device descriptor.
 * @param vector APIC interrupt vector to use.
 * @param lapic_id local APIC ID of the CPU to route the vector to
 * @return True if MSI was enabled successfully.
 */
bool pci_enable_msi(pci_dev_t device, uint32_t vector, uint32_t lapic_id);

/**
 * @brief Display enumerated PCI devices to the system log.
 */
void pci_display_device_list();

/**
 * @brief Get a list of all enumerated PCI devices.
 * @param list Pointer to array of devices (output).
 * @return Number of devices in the list.
 */
size_t pci_get_device_list(pci_dev_t** list);

/**
 * @brief Enable or disable legacy INTx interrupt signalling.
 * @param device PCI device descriptor.
 * @param enable True = enable, False = disable.
 */
void pci_interrupt_enable(pci_dev_t device, bool enable);

/**
 * @brief Enable MSI-X (extended message signalled interrupts).
 * @param device PCI device descriptor.
 * @param vector APIC interrupt vector to assign.
 * @param entry MSI-X table entry to configure.
 * @param lapic_id local APIC ID of the CPU to route the vector to
 * @return True if MSI-X was enabled successfully.
 */
bool pci_enable_msix(pci_dev_t device, uint32_t vector, uint16_t entry, uint32_t lapic_id);

/**
 * @brief Configure an interrupt for a PCI device (MSI/MSI-X if available).
 *
 * Attempts to enable MSI/MSI-X for the given PCI device, routing the
 * interrupt to the specified Local APIC ID. If the device does not support
 * MSI/MSI-X, falls back to legacy INTx routing.
 *
 * @param name     Human-readable device name (used in logs/debug output).
 * @param dev      The PCI device handle.
 * @param logical_cpu_id Logical CPU ID of the target CPU that should receive the interrupt.
 * @param handler  Interrupt service routine to register.
 * @param context  Opaque pointer passed to the ISR when the interrupt fires.
 *
 * @warning The lapic_id parameter must be a valid logical ID
 *          corresponding to an online CPU. Passing an invalid or
 *          offline CPU ID will result in undefined behaviour and
 *          may cause interrupts to be lost.
 *
 * @return The assigned interrupt vector (64–255 if MSI/MSI-X was used,
 *         or IRQ_START + line if falling back to legacy INTx).
 *
 * @note Drivers should always check the returned vector to confirm which
 *       interrupt mechanism was actually configured.
 */
uint32_t pci_setup_interrupt(const char* name, pci_dev_t dev, uint8_t logical_cpu_id, isr_t handler, void *context);
