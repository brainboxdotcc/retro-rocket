#include "initialisation-functions.h"

typedef void (*init_func_t)(void);

init_func_t init_funcs[] = {
	init_console,		init_heap,		rr_console_init_from_limine,
	init_cores,		init_idt,		init_pci,		init_realtime_clock,
	init_devicenames,	init_keyboard,		init_ide,		init_ahci,
	init_filesystem,	init_iso9660,		init_devfs,		init_fat32,
	NULL,
};

char* init_funcs_names[] = {
	"console",	"heap",			"backbuffer",	"cores",
	"idt",		"pci",			"clock",
	"devicenames",	"keyboard",		"ide",		"ahci",
	"filesystem",	"iso9660",		"devfs",	"fat32",
	NULL,
};


void init()
{
	uint32_t n = 0;
	for (init_func_t* func = init_funcs; *func; ++func) {
		dprintf("Bringing up %s...\n", init_funcs_names[n]);
		(*func)();
		dprintf("Initialisation of %s done!\n", init_funcs_names[n++]);
	}
	rr_terminal_draw_to_backbuffer();
}