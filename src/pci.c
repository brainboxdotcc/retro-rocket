#include <kernel.h>

const char* pci_device_class[] = {
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

const pci_subclass pci_device_sub_class[] = 
{
	{ 0x00, 0x00, 0x00, "Pre-2.0 PCI Non-VGA"} ,
	{ 0x00, 0x01, 0x00, "Pre-2.0 PCI VGA Compatible"} ,
	{ 0x01, 0x00, 0x00, "SCSI Controller"} ,
	{ 0x01, 0x01, 0x00, "IDE Controller"} ,
	{ 0x01, 0x01, 0x80, "PIIX4 IDE Controller" },
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

	{ 0x01, 0x06, 0x00, "AHCI controller" } , 

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

uint32_t pci_size_map[100];
pci_dev_t dev_zero = {0};

bool pci_bus_master(pci_dev_t device) {
	uint32_t pci_command_reg = pci_read(device, PCI_COMMAND);
	if(!(pci_command_reg & PCI_COMMAND_BUS_MASTER)) {
		pci_command_reg |= PCI_COMMAND_BUS_MASTER;
		pci_write(device, PCI_COMMAND, pci_command_reg);
		return true;
	}
	return false;
}

bool pci_not_found(pci_dev_t device) {
	return device.bits == dev_zero.bits;
}

uint8_t pci_bar_type(uint32_t field) {
	return field & 0x1;
}

uint16_t pci_io_base(uint32_t field) {
	return field & (~0x3);
}

uint32_t pci_mem_base(uint32_t field) {
	return field & (~0xf);
}	

uint32_t pci_read(pci_dev_t dev, uint32_t field) {
	// Only most significant 6 bits of the field
	dev.field_num = (field & 0xFC) >> 2;
	dev.enable = 1;
	outl(PCI_CONFIG_ADDRESS, dev.bits);

	// What size is this field supposed to be ?
	uint32_t size = pci_size_map[field];
	if(size == 1) {
		// Get the first byte only, since it's in little endian, it's actually the 3rd byte
		uint8_t t =inb(PCI_CONFIG_DATA + (field & 3));
		return t;
	}
	else if(size == 2) {
		uint16_t t = inw(PCI_CONFIG_DATA + (field & 2));
		return t;
	}
	else if(size == 4){
		// Read entire 4 bytes
		uint32_t t = inl(PCI_CONFIG_DATA);
		return t;
	} else {
		uint32_t t = inl(PCI_CONFIG_DATA);
		return t;
	}
	return 0xffff;
}

/*
 * Write pci field
 */
void pci_write(pci_dev_t dev, uint32_t field, uint32_t value) {
	dev.field_num = (field & 0xFC) >> 2;
	dev.enable = 1;
	// Tell where we want to write
	outl(PCI_CONFIG_ADDRESS, dev.bits);
	// Value to write
	outl(PCI_CONFIG_DATA, value);
}

/*
 * Get device type (i.e, is it a bridge, ide controller ? mouse controller? etc)
 */
uint32_t get_device_type(pci_dev_t dev) {
	uint32_t t = pci_read(dev, PCI_CLASS) << 8;
	return t | pci_read(dev, PCI_SUBCLASS);
}

/*
 * Get secondary bus from a PCI bridge device
 */
uint32_t get_secondary_bus(pci_dev_t dev) {
	return pci_read(dev, PCI_SECONDARY_BUS);
}

/*
 * Is current device an end point ? PCI_HEADER_TYPE 0 is end point
 */
uint32_t pci_reach_end(pci_dev_t dev) {
	uint32_t t = pci_read(dev, PCI_HEADER_TYPE);
	return !t;
}

/*
 * Recursive Scan function
 */
pci_dev_t pci_scan_function(uint16_t vendor_id, uint16_t device_id, uint32_t bus, uint32_t device, uint32_t function, int device_type) {
	pci_dev_t dev = {0};
	dev.bus_num = bus;
	dev.device_num = device;
	dev.function_num = function;
	// If it's a PCI Bridge device, get the bus it's connected to and keep searching
	if(get_device_type(dev) == PCI_TYPE_BRIDGE) {
		pci_scan_bus(vendor_id, device_id, get_secondary_bus(dev), device_type);
	}
	// If type matches, we've found the device, just return it
	if(device_type == -1 || device_type == get_device_type(dev)) {
		uint32_t devid  = pci_read(dev, PCI_DEVICE_ID);
		uint32_t vendid = pci_read(dev, PCI_VENDOR_ID);
		if(devid == device_id && vendor_id == vendid)
			return dev;
	}
	return dev_zero;
}

/*
 * Scan device
 */
pci_dev_t pci_scan_device(uint16_t vendor_id, uint16_t device_id, uint32_t bus, uint32_t device, int device_type) {
	pci_dev_t dev = {0};
	dev.bus_num = bus;
	dev.device_num = device;

	//dprintf("%d:%d class=%04x subclass=%04x type=%08x\n", bus, device, pci_read(dev,PCI_CLASS), pci_read(dev,PCI_SUBCLASS), get_device_type(dev));

	if (get_device_type(dev) == device_type && device_type > 0) {
		return dev;
	}

	if(pci_read(dev,PCI_VENDOR_ID) == PCI_NONE)
		return dev_zero;

	pci_dev_t t = pci_scan_function(vendor_id, device_id, bus, device, 0, device_type);
	if(t.bits)
		return t;

	if(pci_reach_end(dev))
		return dev_zero;

	for(int function = 1; function < FUNCTION_PER_DEVICE; function++) {
		if(pci_read(dev,PCI_VENDOR_ID) != PCI_NONE) {
			t = pci_scan_function(vendor_id, device_id, bus, device, function, device_type);
			if(t.bits)
				return t;
		}
	}
	return dev_zero;
}

/*
 * Scan bus
 */
pci_dev_t pci_scan_bus(uint16_t vendor_id, uint16_t device_id, uint32_t bus, int device_type) {
	for(int device = 0; device < DEVICE_PER_BUS; device++) {
		pci_dev_t t = pci_scan_device(vendor_id, device_id, bus, device, device_type);
		if(t.bits)
			return t;
	}
	return dev_zero;
}

/*
 * Device driver use this function to get its device object(given unique vendor id and device id)
 */
pci_dev_t pci_get_device(uint16_t vendor_id, uint16_t device_id, int device_type) {

	pci_dev_t t = pci_scan_bus(vendor_id, device_id, 0, device_type);
	if(t.bits)
		return t;

	// Handle multiple pci host controllers

	if(pci_reach_end(dev_zero)) {
		dprintf("PCI Get device failed\n");
	}
	for(int function = 1; function < FUNCTION_PER_DEVICE; function++) {
		pci_dev_t dev = {0};
		dev.function_num = function;

		if(pci_read(dev, PCI_VENDOR_ID) == PCI_NONE)
			break;
		t = pci_scan_bus(vendor_id, device_id, function, device_type);
		if(t.bits)
			return t;
	}
	return dev_zero;
}

/*
 * PCI Init, filling size for each field in config space
 */
void init_pci() {
	// Init size map
	pci_size_map[PCI_VENDOR_ID] 		= 2;
	pci_size_map[PCI_DEVICE_ID] 		= 2;
	pci_size_map[PCI_COMMAND]		= 2;
	pci_size_map[PCI_STATUS]		= 2;
	pci_size_map[PCI_SUBCLASS]		= 1;
	pci_size_map[PCI_CLASS]			= 1;
	pci_size_map[PCI_CACHE_LINE_SIZE]	= 1;
	pci_size_map[PCI_LATENCY_TIMER]		= 1;
	pci_size_map[PCI_HEADER_TYPE] 		= 1;
	pci_size_map[PCI_BIST] 			= 1;
	pci_size_map[PCI_BAR0] 			= 4;
	pci_size_map[PCI_BAR1] 			= 4;
	pci_size_map[PCI_BAR2] 			= 4;
	pci_size_map[PCI_BAR3] 			= 4;
	pci_size_map[PCI_BAR4] 			= 4;
	pci_size_map[PCI_BAR5]			= 4;
	pci_size_map[PCI_CAPABILITIES] 		= 4;
	pci_size_map[PCI_INTERRUPT_LINE]	= 1;
	pci_size_map[PCI_SECONDARY_BUS]		= 1;
}

bool pci_enable_msi(pci_dev_t device, uint32_t vector, bool edgetrigger, bool deassert)
{
	uint32_t status = pci_read(device, PCI_STATUS);

	/* Check for MSI capability */
	if (!(status & 0x10)) {
		return false;
	}

	uint32_t capabilities_ptr = pci_read(device, PCI_CAPABILITIES);

	uint32_t current = capabilities_ptr, config_space;
	while ((config_space = pci_read(device, current))) {
		uint8_t id = config_space & 0xFF;
		uint16_t device_id = pci_read(device, PCI_DEVICE_ID);
		uint16_t vendor_id = pci_read(device, PCI_VENDOR_ID);
		uint32_t next_capability = (config_space & 0xFF00) >> 8;
		if (id == PCI_CAPABILITY_MSI) {
			/* MSI capability */
			asm volatile("cli");
			uint32_t new_message_data = (vector & 0xFF) | (edgetrigger ? 0 : PCI_MSI_EDGETRIGGER) | (deassert ? 0 : PCI_MSI_DEASSERT);
			uint32_t new_message_address = (0xFEE00000 | (cpu_id() << 12));
			bool bits64cap = (config_space & PCI_MSI_64BIT);
			dprintf("Enable MSI for %04x:%04x with data=%08x address=%08x vector %d\n", vendor_id, device_id, new_message_data, new_message_address, vector);
			pci_write(device, current + 0x04, new_message_address);
			uint16_t device_command_flags = pci_read(device, PCI_COMMAND);
			if (!(device_command_flags & PCI_COMMAND_INTERRUPT_DISABLE)) {
				dprintf("MSI: Legacy interrupts for %04x:%04x disabled\n", vendor_id, device_id);
				pci_write(device, PCI_COMMAND, device_command_flags | PCI_COMMAND_INTERRUPT_DISABLE);
			}
			if (bits64cap) {
				pci_write(device, current + 0x08, 0);
				pci_write(device, current + 0x0C, new_message_data);
			} else {
				pci_write(device, current + 0x08, new_message_data);	
			}
			pci_write(device, current + 0x00, config_space | PCI_MSI_ENABLE); // Mask in enable bit
			asm volatile("sti");
			return true;
		}
		current = next_capability;
		if (next_capability == 0) {
			break;
		}
	}
	return false;
}
