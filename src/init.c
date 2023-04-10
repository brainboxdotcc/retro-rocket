#include "initialisation-functions.h"

typedef void (*init_func_t)(void);

init_func_t init_funcs[] = {
	init_console,		init_heap,		init_cores,		init_idt, 
	init_error_handler,	init_pci,		init_realtime_clock,	init_lapic_timer,
	init_devicenames,	init_keyboard,		init_ide,		init_ahci,
	init_filesystem,	init_iso9660,		init_devfs,		init_fat32,
	NULL,
};

void init()
{
	for (init_func_t* func = init_funcs; *func; ++func) {
		(*func)();
	}

}