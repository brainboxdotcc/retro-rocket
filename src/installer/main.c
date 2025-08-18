#include <installer.h>

const char* choose_drive() {
	new_page("Retro Rocket Installer");
	const storage_device_t* storage_devices = get_all_storage_devices();
	centre_text("Select a drive to install Retro Rocket to by pressing the number keys.\n\n\n");
	warning("WARNING: ALL DATA ON THE TARGET DRIVE WILL BE LOST!", NULL);
	kprintf("\n\n\n\n");
	size_t index = 1;
	static const char* devices[256] = { 0 };
	uint64_t x, y, h;
	get_text_position(&x, &y);
	for (const storage_device_t* cur = storage_devices; cur; cur = cur->next) {
		char summary[MAX_STRINGLEN];
		bool usable = !cur->ui.is_optical && probe_device_summary(cur, summary, sizeof(summary), NULL);
		if (usable) {
			devices[index] = cur->name;
			kprintf("    %s%lu%s: (%s) %s\n", VGA_FG_YELLOW, index++, VGA_RESET, cur->name, cur->ui.label);
			kprintf("       %s%s%s\n\n", VGA_FG_YELLOW, summary, VGA_RESET);
		}
	}
	kprintf("\n");
	get_text_position(&x, &h);
	kprintf("\n");
	h -= y;
	draw_box_cp437_double(3, y - 1, get_text_width() - 6, h + 2);
	kprintf("\n   Your choice (or %s0%s to exit): ", VGA_FG_YELLOW, VGA_RESET);
	uint8_t selected = numeric_choice(0, index - 1);
	if (selected == 0) {
		reboot();
	}
	return devices[selected];
}

void confirm_install(const char* device) {
	new_page("Retro Rocket Installer");
	char prompt[MAX_STRINGLEN];
	kprintf("\n\n\n\n");
	snprintf(prompt, MAX_STRINGLEN, "Are you sure you want to install Retro Rocket to %s?", device);
	warning(prompt, "The drive will be cleared, and any existing data will be lost!");
	kprintf("\n\n\n\n");
	numeric_choice(99, 99);
}

void installer() {
	const char* device = choose_drive();

	confirm_install(device);

	if (install_gpt_esp_rfs_whole_image(device, EFI_FAT_IMAGE)) {
		copy_userland(device);
	}
	wait_forever();
}