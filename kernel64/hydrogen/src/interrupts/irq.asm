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

section .text
bits 64

;-------------------------------------------------------------------------------
; Interrupts - IRQs
;-------------------------------------------------------------------------------

; Wires the IRQs to their vectors in the I/O APICs.
irq_wire:
	; Store
	push rax
	push rcx
	push rdx
	push rsi
	push rdi
	push r8
	push r9

	; Get IRQ table address
	mov r9, qword [config_table]
	mov r9, qword [r9 + hydrogen_config_table.irq_table]

	; Iterate on IRQs
	xor r8, r8				; Current IRQ number

.next:
	; Get GSI number
	mov rsi, info_table.irq_to_gsi
	mov rax, r8
	shl rax, 2				; IRQ * 4 = offset in map
	add rsi, rax
	mov ecx, dword [rsi]

	; Get vector
	xor rdx, rdx
	mov dl, byte [r9]

	; Get entry and value
	call ioapic_entry_get
	call ioapic_entry_read

	; Set vector and write
	and rax, ~IOAPIC_REDIR_VECTOR_MASK
	or rax, rdx
	call ioapic_entry_write

	; Next?
	inc r8
	add r9, hydrogen_config_irq_entry.end
	cmp r8, IRQ_COUNT
	jl .next

	; Restore
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax
	ret

; Set IRQ masks as given in the config table.
;
; Assumes all IRQs are currently masked and interrupts are disabled
; and won't be enabled anymore before the jump into the kernel.
irq_set_masks:
	; Store
	push rax
	push rcx
	push rsi

	; Get IRQ table address
	mov rsi, qword [config_table]
	mov rsi, qword [rsi + hydrogen_config_table.irq_table]

	; Iterate on IRQs
	xor rcx, rcx				; Current IRQ number
.next:
	; Mask flag set?
	mov rax, qword [rsi + hydrogen_config_irq_entry.flags]
	and rax, HYDROGEN_CONFIG_IRQ_FLAG_MASK
	cmp rax, 0
	jne .mask_set

	; Unmask IRQ
	call irq_unmask

.mask_set:
	; Next?
	add rsi, hydrogen_config_irq_entry.end
	inc rcx
	cmp rcx, IRQ_COUNT
	jl .next

	; Restore
	pop rsi
	pop rcx
	pop rax
	ret

; Macro for functions altering an redirection entry of an installed I/O APIC.
;
; %1	Instruction, like or, and or mov.
; %2	Value.
%macro __irq_alter_redir_entry 2
	; Store
	push rax
	push rbx
	push rcx
	push rsi
	push rdi

	; Get GSI number
	shl rcx, 2
	mov rsi, info_table.irq_to_gsi
	add rsi, rcx
	mov ecx, dword [rsi]

	; Get entry and entry value
	call ioapic_entry_get
	call ioapic_entry_read

	; Mask entry
	mov rbx, %2
	%1 rax, rbx

	; Write entry
	call ioapic_entry_write

	; Restore
	pop rdi
	pop rsi
	pop rcx
	pop rbx
	pop rax
	ret
%endmacro

; Masks the IRQ with the given index.
;
; Parameters:
;	rcx The index of the IRQ to mask.
irq_mask: __irq_alter_redir_entry or, (1 << IOAPIC_REDIR_MASK_OFFSET)

; Unmasks the IRQ with the given index.
;
; Parameters:
;	rcx The index of the IRQ to mask.
irq_unmask: __irq_alter_redir_entry and, ~(1 << IOAPIC_REDIR_MASK_OFFSET)
