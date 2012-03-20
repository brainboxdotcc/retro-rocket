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
; Info Table
;-------------------------------------------------------------------------------

; Prepares the info table by writing default data.
info_prepare:
	; Store
	push rax
	push rdi

	; Write default IRQ to GSI mapping
	xor rax, rax
	mov rdi, info_table.irq_to_gsi

.irq2gsi_next:
	stosd
	inc rax
	cmp rax, IRQ_COUNT
	jl .irq2gsi_next

	; Restore
	pop rdi
	pop rax
	ret
