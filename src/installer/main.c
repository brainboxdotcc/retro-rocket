/**
 * @file installer/main.c
 * @author Craig Edwards
 * @brief Retro Rocket OS Installer
 * @copyright Copyright (c) 2012-2025
 */
#include <installer.h>
#include <stdnoreturn.h>

const char* choose_drive() {
	new_page("Retro Rocket Installer");
	const storage_device_t* storage_devices = get_all_storage_devices();
	centre_text("Select a drive to install Retro Rocket to by pressing the number keys.\n\n\n");
	warning("WARNING: ALL DATA ON THE TARGET DRIVE WILL BE LOST!", NULL, VGA_FG_LIGHTWHITE VGA_BG_LIGHTRED);
	vertical_tab();
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
		dprintf("Device: %s\n", cur->ui.label);
		//storage_disable_cache((storage_device_t*)cur);
		dprintf("Device cache disabled\n");
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

bool confirm_install(const char* device) {
	new_page("Retro Rocket Installer");
	char prompt[MAX_STRINGLEN];
	vertical_tab();
	snprintf(prompt, MAX_STRINGLEN, "Are you sure you want to install Retro Rocket to %s?", device);
	warning(prompt, "The drive will be cleared, and any existing data will be lost!", VGA_FG_LIGHTWHITE VGA_BG_LIGHTRED);
	vertical_tab();
	kprintf("\n   Press %sY%s to continue or %sN%s to return to device selection: ", VGA_FG_YELLOW, VGA_RESET, VGA_FG_YELLOW, VGA_RESET);
	return boolean_choice();
}

void display_progress(const char* message, int progress) {
	new_page("Retro Rocket Installer");
	size_t size = strlen_ansi(message), half_size = size / 2;
	size_t third_height = get_text_height() / 3, half_width = get_text_width() / 2;
	draw_text_box_cp437_center(half_width - half_size - 2, third_height, size + 4, message);
	draw_progress_bar_cp437(10, third_height * 2, get_text_width() - 20, progress);
	rr_flip();
}

_Noreturn void reboot_page() {
	set_video_auto_flip(true);
	new_page("Retro Rocket Installer");
	vertical_tab();
	warning("Installation completed!", "Remove CD and press any key to reboot.", VGA_FG_LIGHTWHITE VGA_BG_GREEN);
	while (kgetc() == 255) { __asm__("hlt"); }
	reboot();
}

_Noreturn void error_page(const char* fmt, ...) {
	char error[MAX_STRINGLEN];
	set_video_auto_flip(true);
	va_list args;
	va_start(args, fmt);
	vsnprintf(error, MAX_STRINGLEN - 1, fmt, args);
	va_end(args);
	new_page("Retro Rocket Installer");
	vertical_tab();
	warning("Installation error", error, VGA_FG_YELLOW VGA_BG_LIGHTRED);
	while (kgetc() == 255) { __asm__("hlt"); }
	reboot();
}



void installer() {
	const char* device = NULL;
	do {
		device = choose_drive();
	} while (!confirm_install(device));

	set_video_auto_flip(false);
	if (install_gpt_esp_rfs_whole_image(device, EFI_FAT_IMAGE)) {
		copy_userland(device);
	}
	set_video_auto_flip(true);

	reboot_page();
	wait_forever();
}