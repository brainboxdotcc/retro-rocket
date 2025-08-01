#include "initialisation-functions.h"

typedef void (*init_func_t)(void);

spinlock_t console_spinlock = 0;
spinlock_t debug_console_spinlock = 0;

init_func_t init_funcs[] = {
	init_heap, validate_limine_page_tables_and_gdt, init_console,
	init_acpi, init_idt, boot_aps, init_pci, init_realtime_clock,
	init_devicenames, init_keyboard, init_ide, init_ahci,
	init_filesystem, init_iso9660, init_devfs, init_fat32,
	init_rtl8139, init_e1000,
	NULL,
};

char* init_funcs_names[] = {
	"heap",		"gdt",			"console",	"acpi",
	"idt",		"cpus",			"pci",		"clock",
	"devicenames",	"keyboard",		"ide",		"ahci",
	"filesystem",	"iso9660",		"devfs",	"fat32",
	"rtl8139",	"e1000",
	NULL,
};


void init()
{
	init_spinlock(&debug_console_spinlock);
	init_spinlock(&console_spinlock);
	uint32_t n = 0;
	for (init_func_t* func = init_funcs; *func; ++func) {
		dprintf("Bringing up %s...\n", init_funcs_names[n]);
		(*func)();
		dprintf("Initialisation of %s done!\n", init_funcs_names[n++]);
	}
}