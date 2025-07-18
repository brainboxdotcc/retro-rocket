#include <kernel.h>
#include <ata.h>

/**
 * This is a stub! IDE/ATA support has been removed in preference of SATA AHCI only.
 */
void init_ide() {
	pci_dev_t ata_device = pci_get_device(0, 0, 0x0101);
	if (ata_device.bits) {
		kprintf("IDE devices are not supported by Retro Rocket\n");
	}
}

