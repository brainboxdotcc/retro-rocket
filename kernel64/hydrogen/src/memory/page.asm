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
; Memory - Paging
;-------------------------------------------------------------------------------

; PML4
; PML3	PDP
; PML2	PD
; PML1 	PT
; PML0	Page

struc page_index_group
	.pte				RESB 2
	.pde				RESB 2
	.pdpe				RESB 2
	.pml4e 				RESB 2
	.end:
endstruc

; Maps a page to a 4kB sized physical frame of memory.
;
; Warning: Does not support 2MB mapped areas!
;
; Parameters:
;	rsi The virtual address to map.
; 	rdi The physical address to map to.
;	rdx The flags to map the page with.
page_map:
	; Store
	push rax
	push rbx
	push rcx
	push rsi
	push rdi
	push r8

	; Calculate PTE value
	mov rax, rdi
	or rax, rdx
	or rax, PAGE_FLAG_PRESENT

	; Calculate the index group
	sub rsp, page_index_group.end		; Place to put the index group
	mov r8, rax						; Backup rax
	mov rdi, rsp
	mov rax, rsi
	call page_index_group_calc
	mov rax, r8

	; Create containg tables and set PTE
	mov rbx, cr3		; Load PML4
	mov rsi, rbx		; And use it as parent structure
	mov rcx, 3			; Begin with PDP
	call page_create_and_set

	; Remove index group from stack
	add rsp, page_index_group.end

	; Restore
	pop r8
	pop rdi
	pop rsi
	pop rcx
	pop rbx
	pop rax
	ret

; Creates the containing structures for a page and sets its PTE.
;
; Should originally be called with rcx = 3.
;
; Parameters:
;	rax The value to set in the page's PTE.
;	rcx	The current level of page models.
;	rsi Pointer to the parent structure (PML4 for PDP etc.)
;   rdi Pointer to structure containing the indices into the tables.
page_create_and_set:
	; Store
	push rbx
	push rcx
	push rdx
	push rsi

	; Get index for level
	mov rbx, rcx
	shl rbx, 1				; level * 2 to get offset in index group
	add rbx, rdi			; + base address of index group
	mov bx, word [rbx]
	and rbx, 0xFFFF

	; Get entry in parent table
	shl rbx, 3				; index * 8 to get offset in parent table
	add rbx, rsi			; + base address of parent table
	mov rdx, qword [rbx]	; Load entry

	; Check page model level
	cmp rcx, 0
	je .pte

	; Is present?
	and rdx, PAGE_FLAG_PRESENT
	cmp rdx, 0
	jne .present

	; Allocate a new frame and map the entry to it
	xchg rax, rdx
	call frame_alloc
	xchg rax, rdx
	or rdx, PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE
	mov qword [rbx], rdx

.present:
	; Recurse
	mov rbx, qword [rbx]			; Get current level's structure
	and rbx, PAGE_PHYSICAL_MASK
	mov rsi, rbx					; and use it as the parent table
	dec rcx							; Advance to lower level
	call page_create_and_set		; Recurse

.end:
	; Restore
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	ret

.pte:
	; Write PTE
	mov qword [rbx], rax
	jmp .end

; Calculates an index group for a given address.
;
; Parameters:
;	rax The address to calculate the group for.
;	rdi	The address to put the index group to.
page_index_group_calc:
	; Store
	push rax
	push rbx
	push rdi

	; Store address in rbx and use rax for writing
	; the indices
	xchg rax, rbx

	; Calculate PTE index
	mov rax, rbx
	shr rax, 12
	and rax, 0x1FF
	stosw

	; Calculate PDE index
	mov rax, rbx
	shr rax, 21
	and rax, 0x1FF
	stosw

	; Calculate PDPE index
	mov rax, rbx
	shr rax, 30
	and rax, 0x1FF
	stosw

	; Calculate PML4E index
	mov rax, rbx
	shr rax, 39
	and rax, 0x1FF
	stosw

	; Restore
	pop rdi
	pop rbx
	pop rax
	ret
