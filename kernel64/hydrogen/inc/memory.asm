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
; Hydrogen API - Memory Layout
;-------------------------------------------------------------------------------

%define HYDROGEN_MEMORY					0x104000
%define HYDROGEN_MEMORY_INFO			HYDROGEN_MEMORY + 0x0
%define HYDROGEN_MEMORY_INFO_PROC		HYDROGEN_MEMORY + 0x01000
%define HYDROGEN_MEMORY_INFO_MODS		HYDROGEN_MEMORY + 0x02000
%define HYDROGEN_MEMORY_INFO_MMAP		HYDROGEN_MEMORY + 0x03000
%define HYDROGEN_MEMORY_INFO_IOAPIC		HYDROGEN_MEMORY + 0x04000
%define HYDROGEN_MEMORY_INFO_STRINGS	HYDROGEN_MEMORY + 0x05000

%define HYDROGEN_MEMORY_SYS_IDT64		HYDROGEN_MEMORY + 0x06000
%define HYDROGEN_MEMORY_SYS_GDT64		HYDROGEN_MEMORY + 0x07000

%define HYDROGEN_MEMORY_PAGE_PML4		HYDROGEN_MEMORY + 0x09000
%define HYDROGEN_MEMORY_PAGE_PDP_IDN	HYDROGEN_MEMORY + 0x0A000
%define HYDROGEN_MEMORY_PAGE_PD_IDN		HYDROGEN_MEMORY + 0x0B000
%define HYDROGEN_MEMORY_PAGE_PDP_KERNEL	HYDROGEN_MEMORY + 0x4B000
%define HYDROGEN_MEMORY_PAGE_PD_KERNEL	HYDROGEN_MEMORY + 0x4C000
%define HYDROGEN_MEMORY_PAGE_PT_KERNEL	HYDROGEN_MEMORY + 0x4D000

%define HYDROGEN_MEMORY_STACK			HYDROGEN_MEMORY + 0x4E000
%define HYDROGEN_MEMORY_END				HYDROGEN_MEMORY + 0x6F000

;-------------------------------------------------------------------------------
; Hydrogen API - Other addresses
;-------------------------------------------------------------------------------

%define HYDROGEN_MEMORY_SCREEN_PADDR	0xb8000
