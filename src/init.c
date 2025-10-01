#include "initialisation-functions.h"

typedef void (*init_func_t)(void);

spinlock_t console_spinlock = 0;
spinlock_t debug_console_spinlock = 0;

init_func_t init_funcs[] = {
	validate_limine_page_tables_and_gdt, init_heap, init_console,
	init_acpi, init_idt, boot_aps, init_pci, init_realtime_clock,
	init_devicenames, init_keyboard, init_ide, init_ahci,
	init_filesystem, init_devfs, init_iso9660, init_fat32,
	init_rfs, init_modules, network_up, audio_init,
	NULL,
};

char* init_funcs_names[] = {
	"gdt",		"heap",		"console",	"acpi",
	"idt",		"cpus",		"pci",		"clock",
	"devicenames",	"keyboard",	"ide",		"ahci",
	"filesystem",	"devfs",	"iso9660",	"fat32",
	"rfs",		"modules",	"network",	"audio",
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
#ifdef PROFILE_KERNEL
	setforeground(COLOUR_ORANGE);
	kprintf("THIS IS A PROFILING BUILD - Expect things to run slower!\n");
	setforeground(COLOUR_WHITE);
	serial_init(COM1);
	profile_init(kmalloc(sizeof(profile_entry) * PROFILE_MAX_FUNCS), kmalloc(sizeof(profile_edge) * PROFILE_MAX_EDGES));
	dprintf("Initialising profiler done!\n");
#endif
}