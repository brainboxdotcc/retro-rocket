#include "serial.h"
#include <kernel.h>

/* Wait until transmit holding register is empty */
static inline void serial_wait_tx(uint16_t port) {
	while ((inb(port + 5) & 0x20) == 0);
}

void serial_init(uint16_t port) {
	dprintf("Initialising serial %03x\n", port);
	outb(port + 1, 0x00); // Disable interrupts
	outb(port + 3, 0x80); // Enable DLAB (set baud rate divisor)
	outb(port + 0, 0x03); // Divisor low byte (38400 baud)
	outb(port + 1, 0x00); // Divisor high byte
	outb(port + 3, 0x03); // 8 bits, no parity, one stop bit
	outb(port + 2, 0xC7); // Enable FIFO, clear them, 14-byte threshold
	outb(port + 4, 0x0B); // IRQs enabled, RTS/DSR set
	dprintf("Initialising serial %03x done!\n", port);
}

void serial_putc(uint16_t port, char c) {
	if (c == '\n') {
		serial_putc(port, '\r'); // CRLF for terminals
	}
	serial_wait_tx(port);
	outb(port, (uint8_t)c);
}

void serial_write(uint16_t port, const char *s) {
	while (*s) {
		serial_putc(port, *s++);
	}
}

void serial_printf(uint16_t port, const char *fmt, ...) {
	char buf[MAX_STRINGLEN]; // temporary buffer per call
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (len > 0) {
		if (len >= (int)sizeof(buf)) {
			len = sizeof(buf) - 1;
		}
		buf[len] = '\0';
		serial_write(port, buf);
	}
}
