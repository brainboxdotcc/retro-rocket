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
	u32	vendordevice;
	u8	headertype;
	u8	deviceclass;
	u8	devicesubclass;
	u8	deviceif;
	u8 irq;
	u32	bar[6];
	u32	restype[6];
	u8	bus;
	u8	slot;
	u8	func;
	struct PCI_DeviceTag* next;
} PCI_Device;

void init_pci();
void list_pci(u8 showbars);
void pci_enable_device(PCI_Device* dev);
PCI_Device* pci_find(u16 bus, u16 slot);

#endif

