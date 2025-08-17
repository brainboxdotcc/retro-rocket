#include <installer.h>

void installer() {
	clearscreen(current_console);
	for (size_t y = 0; y < (get_text_height() / 2) - 10; ++y) {
		kprintf("\n");
	}
	centre_text("%sRetro Rocket Installer%s\n\n", VGA_FG_YELLOW, VGA_RESET);
	const storage_device_t* storage_devices = get_all_storage_devices();
	centre_text("Select a drive to install Retro Rocket to by pressing the number keys.\n\n");
	warning("WARNING: ALL DATA ON THE TARGET DRIVE WILL BE LOST!");
	kprintf("\n");
	size_t index = 1;
	const char* devices[256] = { 0 };
	for (const storage_device_t* cur = storage_devices; cur; cur = cur->next) {
		char summary[MAX_STRINGLEN];
		bool usable = !cur->ui.is_optical && probe_device_summary(cur, summary, sizeof(summary), NULL);
		if (usable) {
			devices[index] = cur->name;
			kprintf("    %s%lu%s: (%s) %s\n", VGA_FG_YELLOW, index++, VGA_RESET, cur->name, cur->ui.label);
			kprintf("       %s%s%s\n\n", VGA_FG_YELLOW, summary, VGA_RESET);
		}
	}
	kprintf("\nYour choice: ");
	uint8_t selected = numeric_choice(1, index - 1);

	if (install_gpt_esp_rfs_whole_image(devices[selected], EFI_FAT_IMAGE)) {
		copy_userland(devices[selected]);
	}
	wait_forever();
}