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
; Multiboot - Info Table
;-------------------------------------------------------------------------------

; Parses the multiboot info table.
multiboot_parse:
	; Store
	push rax
	push rcx
	push rsi
	push rdi

	; Get multiboot info table
	xor rax, rax						; Clear rax to load dword pointer
	mov eax, dword [boot32_mboot]		; Load multiboot info table pointer

	; Parse memory map
	xor rsi, rsi										; Clear rsi for dword
	mov esi, dword [rax + multiboot_info.mmap_addr]		; Load mmap address
	xor rcx, rcx										; Clear rcx for dword
	mov ecx, dword [rax + multiboot_info.mmap_length]	; Load mmap length
	call multiboot_mmap_parse							; Parse memory map

	; Parse module list
	xor rsi, rsi
	mov esi, dword [rax + multiboot_info.mods_addr]		; Load mods address
	xor rcx, rcx
	mov ecx, dword [rax + multiboot_info.mods_count]	; Load mods count
	call multiboot_mods_parse							; Parse module list

	; Restore
	pop rdi
	pop rsi
	pop rcx
	pop rax
	ret

; Parses the memory map at rsi with length rcx and fills the mmap of the
; hydrogen info table.
;
; Parameters:
; 	rsi The address of the memory map to parse.
; 	rcx The length of the memory map in bytes.
multiboot_mmap_parse:
	; Store
	push rax
	push rcx
	push rdx
	push rsi
	push rdi

	; Keep track of entry count
	xor rdx, rdx

	; Load target address
	mov rdi, info_mmap

.next_entry:
	; Increase entry counter
	inc rdx

	; Write begin address
	mov rax, qword [rsi + multiboot_mmap_entry.addr]	; Load begin address
	stosq												; Write begin address

	; Write length
	mov rax, qword [rsi + multiboot_mmap_entry.len]		; Load length
	stosq												; Write length

	; Write availability
	xor rax, rax									; Clear rax to store 1 byte
	mov al, byte [rsi + multiboot_mmap_entry.type]	; Load type
	cmp rax, MULTIBOOT_MMAP_ENTRY_AVL				; Is available?
	jne .not_available

.available:
	mov rax, 1									; Set avl flag
	stosb										; Write
	jmp	.handled								; Entry completely handled

.not_available:
	xor rax, rax								; Clear avl flag
	stosb										; Write

.handled: ; Fall through
	; Get address of new element
	xor rax, rax										; Clear rax for dword
	mov eax, dword [rsi + multiboot_mmap_entry.size]	; Load size
	add rax, 4											; Add length of size
	add rsi, rax										; Add size to pointer

	sub rcx, rax								; Subtract size from mmap length
	cmp rcx, 0									; New entry available?
	jne .next_entry

	; Write entry count
	mov byte [info_table.mmap_count], dl

	; Restore
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax
	ret

; Parses the module list at rsi with rcx entries and fills the hydrogen module
; list.
;
; TODO: Check why the cmdline string seems always to be empty with GRUB2.
;
; Parameters:
; 	rsi	The address of the multiboot mod list.
; 	rcx	The number of entries in the multiboot mod list.
multiboot_mods_parse:
	; Any modules loaded?
	cmp rcx, 0
	jne .parse
	ret

.parse:
	; Store
	push rax
	push rbx
	push rcx
	push rsi
	push rdi

	; Write mod list length length
	mov byte [info_table.mod_count], cl

	; Load target address
	mov rdi, info_mods

.next_entry:
	; Write begin address
	xor rax, rax										; Clear rax for dword
	mov eax, dword [rsi + multiboot_mod_list.mod_start]	; Load start address
	mov rbx, rax										; Store val for len calc
	stosq

	; Write length
	mov eax, dword [rsi + multiboot_mod_list.mod_end]	; Load end address
	sub rax, rbx										; Subtract start address
	stosq

	; Copy string
	push rsi											; Save rsi
	push rdi											; and rdi
	xor rax, rax
	mov eax, dword [rsi + multiboot_mod_list.cmdline]	; Get string address
	mov rsi, rax										; str address to rsi

	mov rdi, qword [info_string_offset]	; Get offset into string table
	mov rax, rdi					 	; Save rdi on rax (to save it later on)
	add rdi, info_strings				; Add address of string table
	call string_copy				 	; Copy the string
	sub rdi, info_strings				; Subtract address to get offset
	mov qword [info_string_offset], rdi	; Update string offset
	pop rdi								; Restore rdi
	pop rsi								; and rsi
	stosq								; Write stored rax to descriptor

	; Next module?
	add rsi, multiboot_mod_list.end	; Add size
	dec rcx							; Decrease remaining module count
	cmp	rcx, 0						; No module left?
	jne .next_entry

	; Restore
	pop rdi
	pop rsi
	pop rcx
	pop rbx
	pop rax
	ret
