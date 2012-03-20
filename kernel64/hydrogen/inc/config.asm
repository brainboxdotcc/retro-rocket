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

; Magic number for configuration header
%define HYDROGEN_CONFIG_MAGIC	0x6FD6B53A

; Flags for hydrogen_config_table
%define HYDROGEN_CONFIG_FLAG_LAPIC_TIMER_ONESHOT  (1 << 0)
%define HYDROGEN_CONFIG_FLAG_LAPIC_TIMER_UNMASKED (1 << 1)

; Configuration table the kernel can use to alter Hydrogen's
; default behavior.
;
; .magic		A magic number (CONFIG_MAGIC).
; .flags		Flags.
; .ap_entry		Entry point for application processors or
;				zero (0) to hlt them.
; .irq_table	Pointer to the IRQ table (16 entries) or
;				zero (0) for defaults.
; .lapic_tmrvec Vector for the LAPIC timer.
; .info_vaddr	The virtual address to map the info table
;				to or zero (0), if it should not be mapped.
; .stack_vaddr	The virtual address to map the stacks to
;				or zero (0), if they should not be mapped.
struc hydrogen_config_table
	.magic:						RESB 4
	.flags:						RESB 4
	.ap_entry:					RESB 8
	.irq_table:					RESB 8
	.lapic_tmrvec:				RESB 1
	.info_vaddr:				RESB 8
	.stack_vaddr:				RESB 8
	.end:
endstruc

; Number of entries in hydrogen_config_table.irq_table
%define  HYDROGEN_CONFIG_IRQ_COUNT		16

; Flags for hydrogen_config_irq_entry
%define  HYDROGEN_CONFIG_IRQ_FLAG_MASK	(1 << 0)

; Entry in the IRQ table.
;
; .vector	The vector to direct the IRQ to.
; .flags	Flags for this IRQ.
struc hydrogen_config_irq_entry
	.vector:					RESB 1
	.flags:						RESB 1
	.end:
endstruc
