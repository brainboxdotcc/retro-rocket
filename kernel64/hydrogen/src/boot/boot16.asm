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
bits 16

; Marks the beginning of the 16 bit boot code
boot16_begin:

; Entry point for the APs starting in real mode.
boot16:
	; Clear cs
	jmp 0x0:.cs_cleared - boot16_begin + 0x1000

.cs_cleared:
	; Activate A20
	in al, 0x92
	or al, 2
	out 0x92, al

	; Load GDT
	lgdt [cs:gdtr32 - boot16_begin + 0x1000]

	; Enable protected mode
	mov eax, cr0
	or al, 1
	mov cr0, eax

	; Jump to 32 bit bootstrap
	jmp 0x8:boot16_trampoline - boot16_begin + 0x1000

gdtr32:
.Limit: dw 0x17
.Base: dd gdt32 - boot16_begin + 0x1000

gdt32:
.Null: dw 0x0000, 0x0000, 0x0000, 0x0000
.Code: dw 0xFFFF, 0x0000, 0x9A00, 0x00CF
.Data: dw 0xFFFF, 0x0000, 0x9200, 0x00CF

bits 32
boot16_trampoline:
	; Setup data segment registers
	mov eax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; Jump to 32 bit bootstrap
	mov eax, boot32
	jmp eax

; Marks the end of the 16 bit boot code
boot16_end:
