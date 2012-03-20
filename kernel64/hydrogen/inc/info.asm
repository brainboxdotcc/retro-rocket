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
; Info Table - Main Table
;-------------------------------------------------------------------------------

; Flag indicating the presence of a 8259 PIC
%define INFO_FLAG_PIC (1 << 0)

; Constants specifying an ISA IRQ's polarity (in its flags)
%define INFO_IRQ_POLARITY_OFFSET	0
%define INFO_IRQ_POLARITY_MASK		11b

%define INFO_IRQ_POLARITY_DEFAULT	00b
%define INFO_IRQ_POLARITY_LOW		11b
%define INFO_IRQ_POLARITY_HIGH		01b

; Constants specifying an ISA IRQ's trigger mode (in its flags)
%define INFO_IRQ_TRIGGER_OFFSET		2
%define INFO_IRQ_TRIGGER_MASK		11b

%define INFO_IRQ_TRIGGER_DEFAULT	00b
%define INFO_IRQ_TRIGGER_EDGE		01b
%define INFO_IRQ_TRIGGER_LEVEL		11b

; Main info structure containing important information about the system.
;
; .free_mem_begin	Beginning of the memory area, where it is free to write to
;					without having to fear to override some data.
; .command_line		The command line string with which the kernel has been loaded.
; .lapic_paddr		The physical address of the LAPIC.
; .flags			The system's flags.
; .proc_count		Number of processors.
; .mod_count		Number of modules.
; .mmap_count		Number of memory map entries.
; .ioapic_count		Number of I/O APICs.
; .irq_to_gsi		ISA IRQ to global system interrupt mappings.
; .irq_flags		Flags for handling the ISA IRQs (one entry for each).
%macro __hydrogen_info_table 0
	.free_mem_begin:	RESB 8
	.command_line:		RESB 8
	.lapic_paddr:		RESB 8
	.flags:				RESB 1
	.proc_count:		RESB 1
	.mod_count:			RESB 1
	.mmap_count:		RESB 1
	.ioapic_count:		RESB 1
	.irq_to_gsi:		RESB 4 * 16
	.irq_flags:			RESB 1 * 16
	.end:
%endmacro

struc hydrogen_info_table
	__hydrogen_info_table
endstruc

;-------------------------------------------------------------------------------
; Info Table - Processor
;-------------------------------------------------------------------------------

%define INFO_PROC_FLAG_PRESENT	(1 << 0)
%define INFO_PROC_FLAG_BSP		(1 << 1)
%define INFO_PROC_FLAG_READY	(1 << 2)

; Info structure describing a processor.
;
; .acpi_id		The ACPI id of the processor.
; .apic_id		The APIC id of the processor.
; .flags 		Flags.
; .lapic_freq	Ticks of the LAPIC timer per second.
struc hydrogen_info_proc
	.acpi_id:					RESB	1
	.apic_id:					RESB	1
	.flags:						RESB	2
	.lapic_freq:				RESB	4
	.end:
endstruc

;-------------------------------------------------------------------------------
; Info Table - I/O APIC
;-------------------------------------------------------------------------------

; Info structure describing an I/O APIC.
;
; .id		The id of the I/O APIC.
; .address	The address of the I/O APIC's memory mapped registers.
; .int_base
struc hydrogen_info_ioapic
	.id:						RESB	1
	.address:					RESB	4
	.int_base:					RESB 	4
	.int_count:					RESB 	1	; TODO: Determine this (max redir)
	.end:
endstruc

;-------------------------------------------------------------------------------
; Info Table - Modules
;-------------------------------------------------------------------------------

; Info structure describing a module.
;
; .begin		The 64 bit physical address the module begins at.
; .length		The length of the module in bytes.
; .cmdline		The path of the module file and its arguments (offset in strings)
struc hydrogen_info_mod
	.begin						RESB	8
	.length						RESB 	8
	.cmdline					RESB	8
	.end:
endstruc

;-------------------------------------------------------------------------------
; Info Table - Memory Map
;-------------------------------------------------------------------------------

; Info structure descibing an entry in the memory map.
;
; .begin		The 64 bit physical address the covered section begins at.
; .length		The length of the covered section.
; .available	One, if the section is available for free usage; zero otherwise.
struc hydrogen_info_mmap
	.begin						RESB	8
	.length						RESB	8
	.available					RESB	1
	.end:
endstruc
