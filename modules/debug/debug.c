#include <kernel.h>
#include "debug.h"

bool EXPORTED MOD_INIT_SYM(KMOD_ABI)(void) {
	return true;
}

bool EXPORTED MOD_EXIT_SYM(KMOD_ABI)(void) {
	return true;
}
