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
; ACPI - Basic Tables
;-------------------------------------------------------------------------------

; The RSDP is a pointer to the RSDT/XSDT and can be discovered by searching for
; its signature 'RSD PTR' in low memory.
;
; .signature	The signature of the RSDP (always 'RSD PTR').
; .checksum		A checksum for the first 20 bytes.
; .oemid
; .revision		The version of the RSDP (zero for 1.0, one for 2.0).
; .rsdt_addr	32 bit physical address of the RSDT.
;
; .length		The length of the whole RSDP (since 2.0).
; .xsdt_addr	64 bit physical address of the XSDT (since 2.0).
; .xchecksum	Checksum for all bytes.
; .reserved
struc acpi_rsdp
	; Version 1.0 and later
	.signature				RESB	8
	.checksum				RESB	1
	.oemid					RESB	6
	.revision				RESB	1
	.rsdt_addr				RESB	4

	; Version 2.0 and later
	.length					RESB	4
	.xsdt_addr				RESB	8
	.xchecksum				RESB	1
	.reserved				RESB	3
endstruc

; Each ACPI table (including the RSDT and XSDT) begins with the SDT header.
;
; .signature	The signature of the table (e.g. 'RSDT').
; .length		The length of the table, including the header.
; .revision		The version of the table.
; .checksum		The checksum for the table.
; .oemid
; .oem_tbl_id
; .oem_rev
; .creator_id
; .creator_rev
struc acpi_sdt_header
	.signature				RESB	4
	.length					RESB	4
	.revision				RESB	1
	.checksum				RESB	1
	.oemid					RESB	6
	.oem_tbl_id				RESB	8
	.oem_rev				RESB	4
	.creator_id				RESB	4
	.creator_rev			RESB	4
endstruc

;-----------------------------------------------------------------------
; ACPI - MADT
;-----------------------------------------------------------------------

; Flag in the MADT, indicating the presence of a 8269 PIC in the system.
%define ACPI_MADT_PCAT_COMPAT		(1 << 0)

; Type id for LAPICs.
%define ACPI_MADT_TYPE_LAPIC		0

; Type id for I/O APICs.
%define ACPI_MADT_TYPE_IOAPIC		1

; Type id for interrupt source overrides (ISO)
%define ACPI_MADT_TYPE_ISO			2

; The Multiple APIC Description Table contains information about the interrupt
; controllers installed into the system.
;
; This structure is followed by a variable size list of APIC devices.
;
; .header		The SDT header.
; .lapic_addr	32 bit physical address a processor can reach its LAPIC at.
; .flags		Flags.
struc acpi_madt
	.header					RESB	36
	.lapic_addr				RESB	4
	.flags					RESB	4
endstruc

; Flag for LAPIC indicating that they are enabled; disabled LAPICs should be
; ignored by the operating system.
%define ACPI_MADT_LAPIC_ENABLED		(1 << 0)

; MADT entry describing a processor and it's LAPIC.
;
; .type			Type of the device (ACPI_MADT_TYPE_LAPIC).
; .length		Length of the entry (8).
; .acpi_id		The ACPI id of the processor.
; .apic_id		The APIC id of the processor.
; .flags		Flags.
struc acpi_madt_lapic
	.type					RESB 	1
	.length					RESB	1
	.acpi_id				RESB	1
	.apic_id				RESB	1
	.flags					RESB	4
endstruc

; MADT entry describing an I/O APIC.
;
; .type 		Type of the device (ACPI_MADT_TYPE_IOAPIC)
; .length		Length of the entry (12)
; .ioapic_id	The id of the I/O APIC.
; .ioapic_addr	The memory address of the I/O APIC's memory-mapped registers.
; .int_base		Global System Interrupt number where the interrupt inputs start.
struc acpi_madt_ioapic
	.type					RESB	1
	.length					RESB	1
	.ioapic_id				RESB	1
	.reserved				RESB	1
	.ioapic_addr			RESB	4
	.int_base				RESB	4
endstruc

; MADT entry for interrupt source overrides.
;
; .type			Type of the entry (APCI_MADT_TYPE_ISO)
; .length		Length of the entry (10)
; .bus			Constant, meaning ISA (0)
; .source		IRQ number
; .gsi			Global system interrupt number.
; .flags		Flags (same as INFO_IRQ_* values)
struc acpi_madt_iso
	.type					RESB 1
	.length					RESB 1
	.bus					RESB 1
	.source					RESB 1
	.gsi					RESB 4
	.flags					RESB 2
endstruc
