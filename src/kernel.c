#include <kernel.h>
#include <initialisation-functions.h>

extern volatile struct limine_kernel_file_request rr_kfile_req;

void kmain()
{
	init();

	bool live_cd = false, root_fs_mounted = false;
	if (rr_kfile_req.response) {
		struct limine_kernel_file_response* kernel_info = rr_kfile_req.response;
		live_cd = (kernel_info->kernel_file->media_type == LIMINE_MEDIA_TYPE_OPTICAL);
	}
	if (live_cd) {
		kprintf("Booting into live CD mode...\n");
		root_fs_mounted = filesystem_mount("/", "cd0", "iso9660");
	} else {
		kprintf("Booting from hard disk installation...\n");
		root_fs_mounted = filesystem_mount("/", "hd0", "rfs");
	}
	if (!root_fs_mounted) {
		preboot_fail("Failed to mount boot drive to VFS!");
	}


	/*load_module("e1000");
	unsigned char ip[4];
	dprintf("Waiting for net to be ready\n");
	while (!gethostaddr(ip)) {
		__asm__ volatile("pause");
	};
	dprintf("listening 2000\n");
	int server = tcp_listen(0, 2000, 5);
	int client = -1;
	if (server < 0) {
		dprintf("Listen failure\n");
	} else {
		dprintf("waiting for client connection\n");
		while ((client = tcp_accept(server)) < 0) {
			__asm__ volatile("pause");
		};
		dprintf("Sending reply\n");
		send(client, "HELLORLD", 8);
		uint64_t ticks = get_ticks();
		while (get_ticks() - ticks < 3000) {
			tcp_idle();
			__asm__ volatile("pause");
		}
		dprintf("Closing client\n");
		closesocket(client);
		dprintf("Closing server\n");
		closesocket(server);
	}*/

	init_process();
}
