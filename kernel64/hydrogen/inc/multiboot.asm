; Hydrogen Operating System
; Copyright (C) 2011 Lukas Heidemann
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;-------------------------------------------------------------------------------
; Multiboot Info Structure
;-------------------------------------------------------------------------------

; Structure for the multiboot info table that is passed by a multiboot 1.0
; bootloader.
;
; .flags 			Flags.
; .mem_lower
; .mem_upper
; .boot_device
; .cmdline			Name of loader binary on disk and arguments.
; .mods_count		Number of loaded modules.
; .mods_addr		32 bit physical address of module list.
; .aout_tabsz
; .aout_strsz
; .aout_addr
; .aout_reserved
; .mmap_length		Length of the memory map.
; .mmap_addr		32 bit physical address of memory map.
; .drives_length
; .drives_addr
; .config_table
; .boot_loader_name	Name of the bootloader.
; .apm_table
; .vbe_control_info
; .vbe_mode
; .vbe_interface_seg
; .vbe_interface_off
; .vbe_interface_len
struc multiboot_info
	.flags:					RESB 4
	.mem_lower:				RESB 4
	.mem_upper:				RESB 4
	.boot_device:			RESB 4
	.cmdline:				RESB 4
	.mods_count:			RESB 4
	.mods_addr:				RESB 4
	.aout_tabsz:			RESB 4
	.aout_strsz:			RESB 4
	.aout_addr:				RESB 4
	.aout_reserved:			RESB 4
	.mmap_length:			RESB 4
	.mmap_addr:				RESB 4
	.drives_length:			RESB 4
	.drives_addr:			RESB 4
	.config_table:			RESB 4
	.boot_loader_name:		RESB 4
	.apm_table:				RESB 4
	.vbe_control_info:		RESB 4
	.vbe_mode_info:			RESB 4
	.vbe_mode:				RESB 4
	.vbe_interface_seg:		RESB 4
	.vbe_interface_off:		RESB 2
	.vbe_interface_len:		RESB 2
	.end:
endstruc

%define MULTIBOOT_MMAP_ENTRY_AVL 1

struc multiboot_mmap_entry
	.size:					RESB 4
	.addr:					RESB 8
	.len:					RESB 8
	.type:					RESB 4
	.end:
endstruc

struc multiboot_mod_list
	.mod_start:				RESB 4
	.mod_end:				RESB 4
	.cmdline:				RESB 4
	.pad:					RESB 4
	.end:
endstruc
