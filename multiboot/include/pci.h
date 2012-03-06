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
	u32int	vendordevice;
	u8int	headertype;
	u8int	deviceclass;
	u8int	devicesubclass;
	u8int	deviceif;
	u8int irq;
	u32int	bar[6];
	u32int	restype[6];
	u8int	bus;
	u8int	slot;
	u8int	func;
	struct PCI_DeviceTag* next;
} PCI_Device;

void init_pci();
void pci_enable_device(PCI_Device* dev);
PCI_Device* pci_find(u16int bus, u16int slot);

#endif

