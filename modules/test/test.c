#include <kernel.h>

bool EXPORTED mod_init(void) {
	dprintf("test.ko: mod_init called!\n");
	return true;
}

void EXPORTED mod_exit(void) {
	dprintf("test.ko: mod_exit called!\n");
}
