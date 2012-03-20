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
; I/O APIC - Registers
;-------------------------------------------------------------------------------

; Memory Mapped Registers
;
; .regsel	Selects a register for the IO window.
; .iowin	Window for manipulating the I/O APIC's internal registers.
struc ioapic
	.regsel:			RESB 4
	.reserved:			RESB 0x10 - 4
	.iowin:				RESB 4
endstruc

; Internal Registers
%define IOAPIC_IOAPICID_INDEX			0x00
%define IOAPIC_IOAPICVER_INDEX			0x01
%define IOAPIC_IOAPICARB_INDEX			0x02
%define IOAPIC_IOREDTBL_OFFSET			0x10

;-------------------------------------------------------------------------------
; I/O APIC - Redirection Table
;-------------------------------------------------------------------------------

%define IOAPIC_REDIR_VECTOR_OFFSET		0
%define IOAPIC_REDIR_VECTOR_MASK		0xFF

%define IOAPIC_REDIR_DELMOD_OFFSET		8
%define IOAPIC_REDIR_DELMOD_MASK		111b

%define IOAPIC_REDIR_DEST_OFFSET		56
%define IOAPIC_REDIR_DEST_MASK			0xFF

%define IOAPIC_REDIR_DESTMOD_OFFSET		11
%define IOAPIC_REDIR_DELIVS_OFFSET		12

%define IOAPIC_REDIR_INTPOL_OFFSET		13
%define IOAPIC_REDIR_INTPOL_LOW			1
%define IOAPIC_REDIR_INTPOL_HIGH		0

%define IOAPIC_REDIR_RIRR_OFFSET		14
%define IOAPIC_REDIR_TRIGGER_OFFSET		15
%define IOAPIC_REDIR_MASK_OFFSET		16
