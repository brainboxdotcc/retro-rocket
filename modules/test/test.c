#include <kernel.h>

bool mod_init(void) {
	dprintf("test.ko: mod_init called!\n");
	return true;
}

void mod_exit(void) {
	dprintf("test.ko: mod_exit called!\n");
}
