#include <kernel.h>
#include <limine-requests.h>
#include <initialisation-functions.h>

void kmain()
{
	init();

	if (!filesystem_mount("/", "cd0", "iso9660")) {
		preboot_fail("Failed to mount boot drive to VFS!");
	}
	filesystem_mount("/devices", NULL, "devfs");
	
	const char* rd = init_ramdisk_from_storage("hd0");
	if (rd) {
		filesystem_mount("/harddisk", rd, "fat32");
	}

	//filesystem_mount("/harddisk", "hd0", "fat32");

	init_debug();
	init_rtl8139();

	/*int fd = _open("/harddisk/long-name-test3.txt", _O_RDWR);
	_write(fd, "TEST", 4);
	if (_lseek(fd, 8192, 0) < 0) {
		dprintf("Can't seek to 8192\n");
	} else {
		_write(fd, "TESTICLE", 8);
	}
	_close(fd);

	fd = _open("/harddisk/long-name-test3.txt", _O_RDONLY);
	dprintf("Opened file %d\n", fd);
	while (_eof(fd) == 0) {
		char buffer;
		_read(fd, &buffer, 1);
	}
	_close(fd);*/


	init_process();
}
