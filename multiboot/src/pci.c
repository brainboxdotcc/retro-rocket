#include <io.h>
#include <kmalloc.h>
#include <string.h>
#include <kprintf.h>
#include <memcpy.h>
#include <pci.h>

PCI_Device* pci_devices = NULL;

void scan_pci_bus(int bus);

const char* pci_class_name(u8int class)
{
	if (class > 0x11)
		class = 0;

	return PCI_DevClass[class];
}

const char* pci_subclass_name(u8int class, u8int subclass, u8int progif)
{
	int search = 0;
	for (; PCI_DevSubClass[search].id != 0xFF; ++search)
	{
		if (class == PCI_DevSubClass[search].id && subclass == PCI_DevSubClass[search].subclass && progif == PCI_DevSubClass[search].progif)
			return PCI_DevSubClass[search].description;
	}
	return "Unknown Device Subclass";
}

void pci_write_config_word(u16int bus, u16int slot, u16int func, u16int offset, u16int value)
{
	u32int address;
	u32int lbus = (u32int)bus;
	u32int lslot = (u32int)slot;
	u32int lfunc = (u32int)func;

	/* Enable bit always set here */
	address = (u32int)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((u32int)0x80000000));
	outl(0xCF8, address);
	outw(0xCFC, value);
}

u32int pci_read_config(u16int bus, u16int slot, u16int func, u16int offset)
{
	u32int address;
	u32int lbus = (u32int)bus;
	u32int lslot = (u32int)slot;
	u32int lfunc = (u32int)func;

	/* create configuration address, enable bit set */
	address = (u32int)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((u32int)0x80000000));

	outl(0xCF8, address);
	return inl(0xCFC);
}

u32int pci_get_vendor_and_device(unsigned short bus, unsigned short slot)
{
	return pci_read_config(bus, slot, 0, PCI_OFS_VENDOR);
}

void pci_enable_device(PCI_Device* dev)
{
	u32int current_setting = pci_read_config(dev->bus, dev->slot, 0, PCI_OFS_STATUS_CMD);
	u32int cmd = (current_setting & 0xffff) | PCI_CMD_PORTIO_ENABLE | PCI_CMD_MEMIO_ENABLE;
	pci_write_config_word(dev->bus, dev->slot, 0, PCI_OFS_STATUS_CMD + 2, cmd & 0xffff);
}

PCI_Device* pci_find(u16int bus, u16int slot)
{
	PCI_Device* cur = pci_devices;
	for (; cur; cur = cur->next)
	{
		if (cur->bus == bus && cur->slot == slot)
			return cur;
	}
	return NULL;
}

void list_pci(u8int showbars)
{
	PCI_Device* cur = pci_devices;
	for (; cur; cur = cur->next)
	{
		kprintf("%04x:%04x [%02x] - on bus %d slot %d: %s\n",
				cur->vendordevice & 0xffff,
				(cur->vendordevice >> 16) & 0xffff,
				cur->headertype,
				cur->bus,
				cur->slot,
				pci_subclass_name(cur->deviceclass, cur->devicesubclass, cur->deviceif));
		if (showbars)
		{
			int bar = 0;
			for (; bar < 6 && cur->bar[bar] != 0; bar++)
			{
				kprintf("    BAR%d: %s %08x\n", bar, (cur->restype[bar] == PCI_BAR_IOPORT) ? "I/O port" : "Memory", cur->bar[bar]);
			}
			if (cur->irq != 0xff && cur->irq != 0x00)
				kprintf("    IRQ %d\n", cur->irq);
		}
	}
}

void init_pci()
{
	scan_pci_bus(0);
	list_pci(1);
}

void scan_pci_bus(int bus)
{
	int slot = 0;
	for (; slot < 63; slot++)
	{
		u32int id = pci_get_vendor_and_device(bus, slot);
		if ((id & 0xffff) != 0xffff)
		{
			u32int flags = pci_read_config(bus, slot, 0, PCI_OFS_FLAGS);
			u32int class = pci_read_config(bus, slot, 0, PCI_OFS_CLASS);
			u32int headertype = (flags >> 16) & 0x7f;
			u32int mf = (flags >> 16) & 0x80;
			
			int bar;

			PCI_Device* dev = kmalloc(sizeof(PCI_Device));
			dev->vendordevice = id;
			dev->headertype = (flags >> 16) & 0xff;
			dev->devicesubclass = (class >> 16) & 0xFF;
			dev->deviceif = (class >> 8) & 0xFF;
			dev->bus = bus;
			dev->slot = slot;
			dev->irq = 0xff;
			dev->deviceclass = (class >> 24) & 0xFF;
			dev->next = pci_devices;

			for (bar = 0; bar < 6; ++bar)
				dev->bar[bar] = 0;

			int maxbars = 6;
			
			if (headertype == 0x00 && !mf)
			{
				u32int irq = pci_read_config(bus, slot, 0, PCI_OFS_IRQ);
				irq &= 0xff;
				dev->irq = irq;
			}
			if (headertype == 0x01 || mf)		/* PCI/PCI bus or multifunction */
			{
				u32int secondary_bus = pci_read_config(bus, slot, 0, PCI_OFS_SECONDARYBUS);

				secondary_bus = (secondary_bus >> 8) & 0xFF;
				if (secondary_bus != bus)
					scan_pci_bus(secondary_bus);
				maxbars = 2;
			}

			for (bar = 0; bar < maxbars; ++bar)
			{
				u32int b = pci_read_config(bus, slot, 0, PCI_OFS_BARS + (bar * 4));
				if ((b & 1) == 0)
				{
					dev->restype[bar] = PCI_BAR_MEMORY;
					dev->bar[bar] = b & 0xFFFFFFF0;
				}
				else
				{
					dev->restype[bar] = PCI_BAR_IOPORT;
					dev->bar[bar] = b & 0xFFFFFFFC;
				}
			}

			pci_devices = dev;
		}
	}
}

