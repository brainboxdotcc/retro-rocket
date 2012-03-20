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

bits 64
section .text

extern kmain
extern kmain_ap
extern exception_handlers
global boot_bsp
global hydrogen_config

; Entry point for the BSP.
boot_bsp:
	mov rax, kmain
	call rax
	jmp $

; Entry point for the AP.
boot_ap:
	mov rax, kmain_ap
	call rax
	jmp $

section .data

; Configuration table for Hydrogen.
;
; Hydrogen searches for the hydrogen_config symbol and parses the table.
hydrogen_config:
	.magic: dd 0x6FD6B53A		; Magic number
	.flags: dd 0x0				; No flags
	.ap_entry: dq boot_ap		; boot_ap as entry point for APs
	.irq_table: dq 0x0	; Default IRQ table
	.lapic_tmrvec: db 0x40		; LAPIC timer vector
	.info_vaddr: dq 0xFFFFFF1000000000
	.stack_vaddr: dq 0xFFFFFF2000000000
