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

;-----------------------------------------------------------------------
; Multiboot Header
;-----------------------------------------------------------------------
section .multiboot
bits 32

MULTIBOOT_FLAG_PAGE_ALIGN	EQU 0x0001
MULTIBOOT_FLAG_MEMORY_INFO	EQU	0x0002
MULTIBOOT_FLAG_VIDEO_MODE	EQU	0x0004
MULTIBOOT_FLAG_AOUT_KLUDGE	EQU	0x0008

MULTIBOOT_MAGIC				EQU	0x1BADB002
MULTIBOOT_FLAGS				EQU MULTIBOOT_FLAG_PAGE_ALIGN | \
								MULTIBOOT_FLAG_MEMORY_INFO
MULTIBOOT_CHECKSUM			EQU -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

multiboot_header:
.Magic:
	dd MULTIBOOT_MAGIC
.Flags:
	dd MULTIBOOT_FLAGS
.Checksum:
	dd MULTIBOOT_CHECKSUM

;-------------------------------------------------------------------------------
; Bootstrap (Protected Mode)
;-------------------------------------------------------------------------------
section .text

; Entry point for both the BSP and the APs.
;
; For the BSP, this function is jumped to directly be the bootloader; for the
; APs the real mode boot routines jump to this.
;
; Make sure the APs do not execute this code in parallel as it is NOT THREAD
; SAFE; wait until an AP has finished, then continue with the next one.
;
; For identification, whether the code is executed by the BSP or an AP, the BSP
; sets [boot32_bsp] to zero after completing the function.
global boot32
boot32:
	cli										; Clear interrupts
	add dword [boot32_stack_next], 0x1000	; Prepare next stack
	mov esp, dword [boot32_stack_next]		; Load stack

	call boot32_pae							; Enable PAE
	call boot32_a20							; Enable A20
	call boot32_long_mode					; Enable LM
	cmp byte [boot32_bsp], 1				; Check whether this is the BSP
	je .bsp

.ap:										; Now we are initializing an AP
	call boot32_gdt							; Load 64 bit GDT
	call boot32_paging						; Enable paging
	jmp 0x08:boot64_ap						; Jump to 64 bit AP entry point

.bsp:										; Now we are initializing the BSP
	mov byte [boot32_bsp], 0x0				; All other CPUs are APs
	mov dword [boot32_mboot], ebx			; Store the multiboot info tbl ptr

	call boot32_gdt_init					; Initialize GDT
	call boot32_gdt							; Load 64 bit GDT
	call boot32_map							; Create the address space
	call boot32_paging						; Enable paging
	jmp 0x08:boot64_bsp						; Jump to 64 bit BSP entry point

;-------------------------------------------------------------------------------
; Bootstrap (Protected Mode) - Utilities
;-------------------------------------------------------------------------------

; Initializes the General Descriptor Table.
boot32_gdt_init:
	; Store
	push eax
	push ecx
	push edi

	; Clear GDT frame
	mov edi, sys_gdt64						; Load GDT address
	xor eax, eax							; Fill with zeroes
	mov ecx, 1024							; 4kB = 1024 * 4 byte
	rep stosd

	; Write Kernel Code descriptor
	sub edi, 0x1000							; Beginning of GDT
	add edi, 0x8							; First descriptor
	stosd									; Write empty dword (eax still 0)
	mov eax, 0x209800						; Kernel Code segment
	stosd

	; Write Kernel Data descriptor
	xor eax, eax							; Clear first dword
	stosd									; Write clear dword
	mov eax, 0x209200						; Kernel Data segment
	stosd

	; Write User Code descriptor
	xor eax, eax							; Clear first dword
	stosd									; Write clear dword
	mov eax, 0x20F800						; User Code segment
	stosd

	; Write User Data descriptor
	xor eax, eax							; Clear first dword
	stosd									; Write clear dword
	mov eax, 0x20F200						; User Data segment
	stosd

	; Restore
	pop edi
	pop ecx
	pop eax
	ret

; Loads the General Descriptor Table.
boot32_gdt:
	lgdt [sys_gdtr64]
	ret

; Enables the Physical Address Extension.
boot32_pae:
	push eax
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax
	pop eax
	ret

; Enables the A20 gate.
boot32_a20:
	push eax
	in al, 0x92
	or al, 2
	out 0x92, al
	pop eax
	ret

; Enables long mode in the CPU's MSRs.
boot32_long_mode:
	push eax
	push ecx

	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	pop ecx
	pop eax
	ret

; Enables paging.
; When long mode is prepared, this changes the CPU into compatibility mode.
boot32_paging:
	push eax

	mov eax, paging_pml4					; Load PML4
	mov cr3, eax

	mov eax, cr0                  			; Enable paging
	or eax, 1 << 31
	mov cr0, eax

	pop eax
	ret

;-------------------------------------------------------------------------------
; Bootstrap (Protected Mode) - Mapping
;-------------------------------------------------------------------------------

; Identity maps the first 512 GB and maps the PML4 to itself in the last PML4E.
boot32_map:
	; Store
	push eax
	push ecx
	push edi

	; Setup structures
	mov edi, paging_pml4					; Clear the PML4
	xor eax, eax							; Fill with zeroes
	mov ecx, 1024							; 4kB = 1024 * 4 byte
	rep stosd

	mov edi, paging_pdp_idn					; Clear the identity PDP
	mov ecx, 1024
	rep stosd

	mov edi, paging_pd_idn					; Clear the identity PDs
	mov ecx, 1024 * 64
	rep stosd

	mov eax, paging_pdp_idn
	or eax, PAGE_FLAG_PW
	mov dword [paging_pml4], eax			; Map identity PDP (PML4E 0)

	mov eax, paging_pml4
	or eax, PAGE_FLAG_PW
	mov dword [paging_pml4 + 0xFF8], eax  ; Rec. map PML4 (PML4E 511)

	; Map 64 identity PDPEs to identity PDs
	mov edi, paging_pdp_idn					; Write to identity PDP
	mov ecx, 64								; Write 64 entries
	mov eax, paging_pd_idn					; Map to identity PDs
	or eax, PAGE_FLAG_PW

.next_pdpe:
	stosd									; Write address and flags
	add edi, 4								; Skip last 4 bytes of address
	add eax, 0x1000							; Advance by one page
	dec ecx									; Decrease number of remaining pds
	cmp ecx, 0								; PD left?
	jne .next_pdpe

	; Map identity PDs to memory
	mov eax, PAGE_FLAG_PW | PAGE_FLAG_PS	; Start at 0x0
	mov edi, paging_pd_idn					; Write to identity PDs
	mov ecx, 512 * 64						; Write 512 entries in 64 PDs

.next_pde:
	stosd									; Write address and flags
	add edi, 4								; Skip last 4 bytes of address
	add eax, 0x200000						; Advance by one 2MB page
	dec ecx									; Decrease number of remaining pages
	cmp ecx, 0								; Page left?
	jne .next_pde

	; Restore
	pop edi
	pop ecx
	pop eax
	ret
