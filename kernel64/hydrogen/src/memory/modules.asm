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
extern end

;-------------------------------------------------------------------------------
; Modules
;-------------------------------------------------------------------------------

; Moves all the modules in the hydrogen info table to the memory after this
; binary.
;
; Sets free_mem_begin to the end of the last module (aligned on a page boundary).
modules_move:
	; Store
	push rbx
	push rcx
	push rsi
	push rdi

	; Load target address
	mov rdi, end

	; Load list address and module count
	xor rcx, rcx
	mov cl, byte [info_table.mod_count]		; Get module count
	mov rsi, info_mods						; Get module list address

	; No modules?
	cmp rcx, 0
	je .end

.next_module:
	; Move module
	align_page rdi							; Page align rdi
	push rcx								; Store rcx and rsi for the call
	push rsi								; to memory_copy
	mov rcx, qword [rsi + hydrogen_info_mod.length]	; Load length
	mov rsi, qword [rsi + hydrogen_info_mod.begin]	; Load address
	;call memory_copy						; Copy the module to the new address
	mov rbx, rdi							; Back up the module's new address
	add rdi, rcx							; Add the module's length to the
											; destination buffer
	pop rsi									; Restore rsi
	pop rcx									; Restore rcx

	; Write the module's new address to the descriptor
	mov qword [rsi + hydrogen_info_mod.begin], rbx

	; Next module?
	add rsi, 24								; Advance to next module
	dec rcx									; Decrease remaining mod count
	cmp rcx, 0								; Module left?
	jne .next_module

.end:
	; Store rdi as end of used memory (page aligned)
	align_page rdi
	mov qword [info_table.free_mem_begin], rdi

	; Restore
	pop rdi
	pop rsi
	pop rcx
	pop rbx
	ret

; Sorts the module list in the hydrogen info table.
modules_sort:
	; Store
	push rbx
	push rcx
	push rsi
	push rdi

	; Load list address and module count
	xor rcx, rcx
	mov cl, byte [info_table.mod_count]		; Get module count
	mov rsi, info_mods						; Get module list address

	; No modules?
	cmp rcx, 0
	je .restore

.next_run:
	; Find first module and swap with first in list
	mov rbx, rsi					; Backup rsi
	call modules_first				; Get first module's address
	mov rdi, rbx					; Beginning of list to rdi for swap
	call modules_swap				; Swap modules at rsi and rdi
	mov rsi, rbx					; Restore rsi

	; Next module?
	add rsi, 24						; Advance to next entry
	dec rcx							; Decrease remaining modules count
	cmp rcx, 0						; No module left?
	jne .next_run

.restore:
	; Restore
	pop rdi
	pop rsi
	pop rcx
	pop rbx
	ret

; Returns the module that begins first in the list starting at rsi with rcx
; entries.
;
; Parameters:
; 	rsi The address of the module list.
;	rcx The number of modules in the list.
;
; Returns:
;	rsi The address of the first module.
modules_first:
	; No module left?
	cmp rcx, 0					; No module left (module count zero)?
	jne .begin
	xor rsi, rsi				; Return null pointer
	ret

.begin:
	; Store
	push rax
	push rbx
	push rcx
	push rdx

	; Find first
	mov rdx, 0xffffffff 		; The start address of the first module.
	xor rbx, rbx				; The address of the first module's entry

.next:
	; Get start address and compare
	lodsq						; Load start address
	sub rsi, 8					; Roll back to beginning of mod entry
	cmp rax, rdx				; Address smaller?
	jl .smaller
	jmp .prepare_next

.smaller:
	; Set as first
	mov rdx, rax
	mov rbx, rsi

	; Fall through
.prepare_next:
	; Advance to next entry
	add rsi, 24					; Advance to next entry
	dec rcx						; Decrease remaining count
	cmp rcx, 0					; Module left?
	jne .next

	; Return smallest
	mov rsi, rbx

	; Restore
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret

; Swaps the two modules descriptors at rsi and rdi.
;
; Parameters:
; 	rsi The address of the first module descriptor.
; 	rdi The address of the second module descriptor.
modules_swap:
	; Exchange all three qwords
	xchg_mem rsi, rdi, qword
	xchg_mem rsi + 8, rdi + 8, qword
	xchg_mem rsi + 16, rdi + 16, qword
	ret
