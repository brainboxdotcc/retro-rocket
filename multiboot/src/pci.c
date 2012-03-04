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

u32int pci_read_config(u16int bus, u16int slot, u16int func, u16int offset)
{
	u32int address;
	u32int lbus = (u32int)bus;
	u32int lslot = (u32int)slot;
	u32int lfunc = (u32int)func;
	//u16int tmp = 0;

	/* create configuration address */
	address = (u32int)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((u32int)0x80000000));
				   
	/* write out the address */
	outl(0xCF8, address);
	
	/* read in the data */
	/* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
	//tmp = (u16int)((inl (0xCFC) >> ((offset & 2) * 8)) & 0xffff);
	return inl(0xCFC);
}

u32int pci_get_vendor_and_device(unsigned short bus, unsigned short slot)
{
	//u16int vendor, device;
	//if ((vendor = pci_read_config(bus, slot, 0, PCI_OFS_VENDOR)) != 0xFFFF)
	//{
	//	device = pci_read_config(bus, slot, 0, PCI_OFS_DEVICE);
	//}
	//return ((u32int)vendor << 16 | device);
	return pci_read_config(bus, slot, 0, PCI_OFS_VENDOR);
}

void list_pci(u8int showbars)
{
	PCI_Device* cur = pci_devices;
	for (; cur;
			cur = cur->next)
	{
		kprintf("%04x:%04x - on bus %d slot %d: %s\n",
				cur->vendordevice & 0xffff,
				(cur->vendordevice >> 16) & 0xffff,
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
			
			int bar;

			PCI_Device* dev = kmalloc(sizeof(PCI_Device));
			dev->vendordevice = id;
			dev->devicesubclass = (class >> 16) & 0xFF;
			dev->deviceif = (class >> 8) & 0xFF;
			dev->bus = bus;
			dev->slot = slot;
			dev->deviceclass = (class >> 24) & 0xFF;
			dev->next = pci_devices;

			for (bar = 0; bar < 6; ++bar)
				dev->bar[bar] = 0;

			int maxbars = 6;
			
			if (headertype == 0x01)	/* PCI/PCI bus or multifunction */
			{
				u32int secondary_bus = pci_read_config(bus, slot, 0, PCI_OFS_SECONDARYBUS);
				kprintf("Secondary: %08x\n", secondary_bus);
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

