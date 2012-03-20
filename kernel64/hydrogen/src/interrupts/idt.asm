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
; Interrupts - IDT
;-------------------------------------------------------------------------------

; Clears the interrupt descriptor table.
int_init:
	; Store
	push rax
	push rcx
	push rdi

	; Clear IDT
	mov rdi, sys_idt64					; Load idt address
	mov rcx, 512						; 4kB = 512 * 8 byte
	xor rax, rax						; Fill with zeroes
	rep stosq

	; Restore
	pop rdi
	pop rcx
	pop rax
	ret

; Loads the IDT into the CPU's registers
int_load:
	lidt [sys_idtr64]
	ret

; Writes an idt entry with the vector given in rcx, using the default code
; segment (0x8) and default flags (0x8E).
;
; Parameters:
;	rcx The vector of the entry to write.
;	rdx	The address of the interrupt handler.
int_write_entry:
	; Store
	push rcx
	push rdx
	push rdi

	; Calculate entry address
	mov rdi, sys_idt64
	shl rcx, 4			; Multiply vector with entry size (16)
	add rdi, rcx

	; Write entry
	mov word [rdi + int_idt_entry.handlerLow], dx
	mov word [rdi + int_idt_entry.cs], 0x8
	mov byte [rdi + int_idt_entry.flags], 0x8E
	shr rdx, 16
	mov word [rdi + int_idt_entry.handlerMiddle], dx
	shr rdx, 16
	mov dword [rdi + int_idt_entry.handlerHigh], edx

	; Restore
	pop rdi
	pop rdx
	pop rcx
	ret
