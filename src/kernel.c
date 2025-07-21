#include <kernel.h>
#include <limine-requests.h>
#include <initialisation-functions.h>

void kmain()
{
	init();

	if (!filesystem_mount("/", "cd0", "iso9660")) {
		preboot_fail("Failed to mount boot drive to VFS!");
	}

	init_rtl8139();
	init_e1000();
	netdev_t* network = get_active_network_device();
	if (network) {
		kprintf("Active network card: %s\n", network->description);
		network_up();
	}

	init_process();
}
