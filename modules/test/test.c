#include <kernel.h>

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	dprintf("test.ko: mod_init called!\n");
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	dprintf("test.ko: mod_exit called!\n");
	return true;
}
