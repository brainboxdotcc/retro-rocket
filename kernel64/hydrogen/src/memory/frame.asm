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
; Memory - Frames
;-------------------------------------------------------------------------------

; Allocates a new 4kB frame of memory.
;
; Uses free_mem_begin to place the frames in unused space; updates the field
; accordingly.
;
; Returns:
;	rax Allocated frame of memory.
frame_alloc:
	; Store
	push rbx
	push rdi

	; Load address
	mov rdi, info_table.free_mem_begin
	mov rax, qword [rdi]

	; Update field
	mov rbx, rax
	add rbx, 0x1000
	mov qword [rdi], rbx

	; Restore
	pop rdi
	pop rbx
	ret
