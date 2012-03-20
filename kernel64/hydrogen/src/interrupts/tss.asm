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

; Creates a TSS for the current processor, inserts a pointer into the
; GDT and loads the TSS.
tss_init:
	; Store
	push rax
	push rbx
	push rdx
	push rdi

	; Get address of TSS
	call smp_id					; Get LAPIC id
	mov rbx, rax				; Store id
	shl rax, 7					; Multiply with 128
	add rax, sys_tss64			; TSS base
	mov rdx, rax

	; Get address of TSS pointer in GDT
	mov rax, rbx				; Get id
	shl rax, 4					; Multiply with 16
	add rax, 5 * 8				; 5 selectors before TSS pointers
	mov rbx, rax				; Backup selector
	add rax, sys_gdt64			; GDT base
	mov rdi, rax

	; Create TSS pointer
	call tss_create_pointer

	; Get stack address
	mov rax, rsp
	and rax, ~0xFFF

	; Create TSS
	mov rdi, rdx
	mov rdx, rax
	call tss_create

	; Update GDT size
	mov rax, rbx
	add rax, 16 - 1			; limit = size - 1 = offset + 16 - 1
	mov word [sys_gdtr64.length], ax

	; Reload GDT
	lgdt [sys_gdtr64]

	; Get selector for TSS
	mov rax, rbx
	ltr ax

	; Restore
	pop rdi
	pop rdx
	pop rbx
	pop rax
	ret

; Creates a TSS pointer at the given address.
;
; Parameters:
;	rdi The address to store the TSS pointer to.
;	rdx The address of the TSS to point to.
tss_create_pointer:
	; Store
	push rax
	push rdi

	; Write limitLow (word)
	mov rax, 13 * 8
	stosw

	; Write base (word)
	mov rax, rdx
	stosw

	; Write base (byte)
	shr rax, 16
	stosb

	; Write flags (word)
	;
	; Flags are:
	;	TSS (0x9)
	;	Present (1 << 7)
	;	Ring 3 (3 << 5)
	mov rax, 0x9 | (1 << 7) | (3 << 5)
	stosw

	; Write base (word)
	mov rax, rdx
	shr rax, 24
	stosw

	; Write base (dword)
	shr rax, 8
	stosd

	; Write reserved (dword)
	xor rax, rax
	stosd

	; Restore
	pop rdi
	pop rax
	ret

; Creates a TSS at the given address.
;
; Parameters:
;	rdi The address to store the TSS to.
;	rdx The ring 0 stack pointer.
tss_create:
	; Store
	push rax
	push rcx
	push rdi

	; Write reserved (dword)
	xor rax, rax
	stosd

	; Write RSP0 (qword)
	mov rax, rdx
	stosq

	; Write remaining (11x qword, 2x word)
	xor rax, rax
	mov rcx, 11
	rep stosq
	stosw
	stosw

	; Restore
	pop rdi
	pop rcx
	pop rax
	ret
