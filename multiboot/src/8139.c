#include <io.h>
#include <kmalloc.h>
#include <string.h>
#include <kprintf.h>
#include <memcpy.h>
#include <pci.h>

static inline void write_register_byte(struct PCI_Device* dev, uint8_t reg, uint8_t value)
{
	outb(dev->bar[0] + reg, value);
}

static inline void write_register_word(struct PCI_Device* dev, uint8_t reg, uint16_t value)
{
	outw(dev->bar[0] + reg, value);
}

static inline void write_register_dword(struct PCI_Device* dev, uint8_t reg, uint32_t value)
{
	outl(dev->bar[0] + reg, value);
}

static inline uint8_t read_register_byte(struct PCI_Device* dev, uint8_t reg)
{
	return inb(dev->bar[0] + reg);
}

static inline uint16_t read_register_word(struct PCI_Device* dev, uint8_t reg)
{
	return inw(dev->bar[0] + reg);
}

static inline uint32_t read_register_dword(struct PCI_Device* dev, uint8_t reg)
{
	return inl(dev->bar[0] + reg);
}

void RTL8139_interrupt(registers_t* regs)
{
}

void init_RTL8139()
{
	PCI_Device* dev = pci_find(0x10ec, 0x8139);
	unsigned char* buf = (unsigned char*)kmalloc(8192);

	if (!dev)
		return;

	register_interrupt_handler(dev->irq, RTL8139_interrupt);

	write_register_byte(dev, REG_COMMAND, CR_RESET);

	while ((read_register_byte(dev, REG_COMMAND) & REG_COMMAND) == CR_RESET);

}
