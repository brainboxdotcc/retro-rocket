#include <io.h>
#include <kmalloc.h>
#include <string.h>
#include <kprintf.h>
#include <memcpy.h>
#include <pci.h>

PCI_Device* pci_devices = NULL;

const char* PCI_DevClass[] = {
	"Unknown",
	"Mass Storage Controller",
	"Network Controller",
	"Display Controller",
	"Multimedia Controller",
	"Memory Controller",
	"Bridge Device",
	"Simple Communication Controller",
	"Base System Peripheral",
	"Input Device",
	"Docking Station",
	"Processor",
	"Serial Bus Controller",
	"Wireless Controller",
	"Intelligent I/O Controller",
	"Satellite Communication Controller",
	"Encryption/Decryption Controller",
	"Data Acquisition/Signal Processing Controller"
};

typedef struct
{
	u8int id;
	u8int subclass;
	u8int progif;
	const char* description;
} SubClass;

const SubClass PCI_DevSubClass[] = 
{
	{ 0x00, 0x00, 0x00, "Pre-2.0 PCI Non-VGA"} ,
	{ 0x00, 0x01, 0x00, "Pre-2.0 PCI VGA Compatible"} ,
	{ 0x01, 0x00, 0x00, "SCSI Controller"} ,
	{ 0x01, 0x01, 0x00, "IDE Controller"} ,
	{ 0x01, 0x02, 0x00, "Floppy Controller"} ,
	{ 0x01, 0x03, 0x00, "IPI Mass Storage Controller"} ,
	{ 0x01, 0x04, 0x00, "RAID Controller"} ,
	{ 0x01, 0x80, 0x00, "Other Mass Storage Controller"} ,
	{ 0x02, 0x00, 0x00, "Ethernet Controller"} ,
	{ 0x02, 0x01, 0x00, "Token Ring Controller"} ,
	{ 0x02, 0x02, 0x00, "FDDI Controller"} ,
	{ 0x02, 0x03, 0x00, "ATM Controller"} ,
	{ 0x02, 0x80, 0x00, "Other Network Controller"} ,

	{ 0x01, 0x05, 0x20, "ATA controller (single DMA)" } , 
	{ 0x01, 0x05, 0x30, "ATA controller (chained DMA)" } ,

	{ 0x03, 0x00, 0x00, "VGA Compatible Display Controller" } ,
	{ 0x03, 0x00, 0x01, "8514 Compatible Display Controller" } ,
	{ 0x03, 0x01, 0x00, "XGA Display Controller" } ,
	{ 0x03, 0x80, 0x00, "Other Display Controller" } ,

	{ 0x04, 0x00, 0x00, "Video Multimedia Device" } ,
	{ 0x04, 0x01, 0x00, "Audio Multimedia Device" } ,
	{ 0x04, 0x80, 0x00, "Other Multimedia Device" } ,

	{ 0x05, 0x00, 0x00, "RAM Memory Controller" } ,
	{ 0x05, 0x01, 0x00, "Flash Memory Controller" } ,
	{ 0x05, 0x80, 0x00, "Other Memory Controller" } ,

	{ 0x06, 0x00, 0x00, "Host/PCI Bridge Device" } ,
	{ 0x06, 0x01, 0x00, "PCI/ISA Bridge Device" } ,
	{ 0x06, 0x02, 0x00, "PCI/EISA Bridge Device" } ,
	{ 0x06, 0x03, 0x00, "PCI/MCA Bridge Device" } ,
	{ 0x06, 0x04, 0x00, "PCI/PCI Bridge Device" } ,
	{ 0x06, 0x05, 0x00, "PCI/PCMCIA Bridge Device" } ,
	{ 0x06, 0x06, 0x00, "PCI/NuBus Bridge Device" } ,
	{ 0x06, 0x07, 0x00, "PCI/CardBus Bridge Device" } ,
	{ 0x06, 0x80, 0x00, "Other Bridge Device" } ,

	{ 0x07, 0x00, 0x00, "Serial Simple Comms Controller" } ,
	{ 0x07, 0x00, 0x01, "Serial 16450 Simple Comms Controller" } ,
	{ 0x07, 0x00, 0x02, "Serial 16550 Simple Comms Controller" } ,
	{ 0x07, 0x01, 0x00, "Parallel Simple Comms Controller" } ,
	{ 0x07, 0x01, 0x00, "Parallel Bi-Directional Simple Comms Controller" } ,
	{ 0x07, 0x01, 0x01, "Parallel ECP Simple Comms Controller" } ,
	{ 0x07, 0x80, 0x02, "Other Simple Comms Controller" } ,

	{ 0x08, 0x00, 0x00, "8259 PIC Base Device" } ,
	{ 0x08, 0x00, 0x01, "ISA PICBase Device" } ,
	{ 0x08, 0x00, 0x02, "PCI PIC Base Device" } ,
	{ 0x08, 0x01, 0x00, "8259 DMA Base Device" } ,
	{ 0x08, 0x01, 0x01, "ISA DMA Base Device" } ,
	{ 0x08, 0x01, 0x02, "EISA DMA Base Device" } ,
	{ 0x08, 0x02, 0x00, "8259 Timer Base Device" } ,
	{ 0x08, 0x02, 0x01, "ISA Timer Base Device" } ,
	{ 0x08, 0x02, 0x02, "EISA Timer Base Device" } ,
	{ 0x08, 0x03, 0x00, "Generic RTC Base Device" } ,
	{ 0x08, 0x03, 0x01, "ISA RTC Base Device" } ,
	{ 0x08, 0x80, 0x00, "Other Base Device" } ,

	{ 0x09, 0x00, 0x00, "Keyboard Input Device" } ,
	{ 0x09, 0x01, 0x00, "Digitizer Input Device" } ,
	{ 0x09, 0x02, 0x00, "Mouse Input Device" } ,
	{ 0x09, 0x80, 0x00, "Other Input Device" } ,

	{ 0x0A, 0x00, 0x00, "Generic Docking Station" } ,
	{ 0x0A, 0x80, 0x00, "Other Docking Station" } ,

	{ 0x0B, 0x00, 0x00, "i386 Processor" } ,
	{ 0x0B, 0x01, 0x00, "i486 Processor" } ,
	{ 0x0B, 0x02, 0x00, "Pentium Processor" } ,
	{ 0x0B, 0x10, 0x00, "Alpha Processor" } ,
	{ 0x0B, 0x20, 0x00, "PowerPC Processor" } ,
	{ 0x0B, 0x80, 0x00, "Co-Processor" } ,

	{ 0x0C, 0x00, 0x00, "IEE1394 Serial Bus Controller" } ,
	{ 0x0C, 0x01, 0x00, "ACCESS.bus Serial Bus Controller" } ,
	{ 0x0C, 0x02, 0x00, "SSA Serial Bus Controller" } ,
	{ 0x0C, 0x03, 0x00, "USB Controller (UHCI)" } ,
	{ 0x0C, 0x03, 0x10, "USB Controller (OHCI)" } ,
	{ 0x0C, 0x03, 0x20, "USB2 Host Controller (Intel EHCI)" },
	{ 0x0C, 0x03, 0x80, "USB Host Controller (Intel EHCI)" },
	{ 0x0C, 0x03, 0xFE, "USB (Not Host Controller)" },
	{ 0x0C, 0x04, 0x00, "Fibre Channel Serial Bus Controller" } ,
	{ 0x0C, 0x05, 0x00, "SMBus Controller" },

	{ 0x0D, 0x00, 0x00, "IRDA Compatible Controller" },
	{ 0x0D, 0x01, 0x00, "Consumer IR Controller" },
	{ 0x0D, 0x10, 0x00, "RF Controller" },
	{ 0x0D, 0x11, 0x00, "Bluetooth Controller" },
	{ 0x0D, 0x12, 0x00, "Broadband Controller" },
	{ 0x0D, 0x20, 0x00, "Ethernet Controller (802.11a)" },
	{ 0x0D, 0x21, 0x00, "Ethernet Controller (802.11b)" },
	{ 0x0D, 0x80, 0x00, "Other Wireless Controller" },

	{ 0x0F, 0x01, 0x00, "TV Controller" },
	{ 0x0F, 0x02, 0x00, "Audio Controller" },
	{ 0x0F, 0x03, 0x00, "Voice Controller" },
	{ 0x0F, 0x04, 0x00, "Data Controller" },

	{ 0x10, 0x00, 0x00, "Network and Computing Encrpytion/Decryption" },
	{ 0x10, 0x10, 0x00, "Entertainment Encryption/Decryption" },
	{ 0x10, 0x80, 0x00, "Other Encryption/Decryption" },

	{ 0xFF, 0xFF, 0xFF, "" }
};


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
	//list_pci(1);
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

