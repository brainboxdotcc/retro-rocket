#include <kernel.h>
#include <limine-requests.h>
#include <initialisation-functions.h>

void kmain()
{
	init();

	if (!filesystem_mount("/", "cd0", "iso9660")) {
		preboot_fail("Failed to mount boot drive to VFS!");
	}

	init_debug();
	init_rtl8139();

	init_process();
}
