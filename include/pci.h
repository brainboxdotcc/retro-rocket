#ifndef __PCI_H__
#define __PCI_H__

#define PCI_OFS_VENDOR		0x00
#define PCI_OFS_DEVICE		0x02
#define PCI_OFS_STATUS_CMD	0x04
#define PCI_OFS_CLASS		0x08
#define PCI_OFS_FLAGS		0x0C
#define PCI_OFS_BARS		0x10
#define PCI_OFS_IRQ		0x3C

#define PCI_OFS_SECONDARYBUS	0x18

#define PCI_BAR_MEMORY		0x01
#define PCI_BAR_IOPORT		0x02

#define PCI_CMD_PORTIO_ENABLE	1 << 0
#define PCI_CMD_MEMIO_ENABLE	1 << 1

typedef struct PCI_DeviceTag
{
	uint32_t	vendordevice;
	uint8_t	headertype;
	uint8_t	deviceclass;
	uint8_t	devicesubclass;
	uint8_t	deviceif;
	uint8_t irq;
	uint32_t	bar[6];
	uint32_t	restype[6];
	uint8_t	bus;
	uint8_t	slot;
	uint8_t	func;
	struct PCI_DeviceTag* next;
} PCI_Device;

void init_pci();
void list_pci(uint8_t showbars);
void pci_enable_device(PCI_Device* dev);
PCI_Device* pci_find(uint16_t bus, uint16_t slot);

#endif

