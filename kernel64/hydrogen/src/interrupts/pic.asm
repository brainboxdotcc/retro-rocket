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
; Interrupts - PIC
;-------------------------------------------------------------------------------

; Initializes the 8259 PIC (if present).
pic_init:
	; Store
	push rax

	; Check if PIC is present
	mov al, byte [info_table.flags]			; Load flags
	and al, INFO_FLAG_PIC					; Check PIC flag
	cmp al, 0
	jne .pic_present

	; Restore
	pop rax
	ret

.pic_present:
	; Store (rax already stored)
	push rdx

	; ICW1: Start initialization sequence
	mov dx, IO_PIC1_COMMAND
	mov al, PIC_ICW1_INIT + PIC_ICW1_ICW4
	out dx, al

	mov dx, IO_PIC2_COMMAND
	mov al, PIC_ICW1_INIT + PIC_ICW1_ICW4
	out dx, al

	; ICW2: Define PIC vectors
	mov dx, IO_PIC1_DATA
	mov al, IRQ_VECTOR
	out dx, al

	mov dx, IO_PIC2_DATA
	mov al, IRQ_VECTOR + 7
	out dx, al

	; ICW3: Continue init sequence
	mov dx, IO_PIC1_DATA
	mov al, 4
	out dx, al

	mov dx, IO_PIC2_DATA
	mov al, 2
	out dx, al

	; ICW4: Additional flags
	mov dx, IO_PIC1_DATA
	mov al, PIC_ICW4_8086
	out dx, al

	mov dx, IO_PIC2_DATA
	mov al, PIC_ICW4_8086
	out dx, al

	; Mask all IRQs
	mov dx, IO_PIC1_DATA
	mov al, 0xFF & ~(1 << 2) ; (line 2 used for cascading)
	out dx, al

	mov dx, IO_PIC2_DATA
	mov al, 0xFF
	out dx, al

	; Restore
	pop rdx
	pop rax
	ret

