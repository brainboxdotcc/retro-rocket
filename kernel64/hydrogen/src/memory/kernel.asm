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

section .data
bits 64

; Searches for the kernel binary in the list of loaded modules.
kernel_find:
	; Store
	push rax
	push rbx
	push rcx
	push rdx
	push rsi

	; No modules?
	xor rcx, rcx
	mov cl, byte [info_table.mod_count]
	cmp cl, 0
	je .not_found

	; Iterate over list
	mov rsi, info_mods

.parse:
	; Cmdline string offset equals zero?
	mov rdx, qword [rsi + hydrogen_info_mod.cmdline]
	cmp rdx, 0
	je .next

	; Add address of string table
	add rdx, info_strings

	; Cmdline begins with kernel module name
	mov rbx, "kernel64"
	mov rax, qword [rdx]
	cmp rbx, rax
	jne .next

	; Found kernel module
	mov qword [kernel_module], rsi
	jmp .found

.next:
	; Check for next
	add rsi, 24				; Add module descriptor size
	dec rcx					; Decrease remaining module count
	cmp rcx, 0
	jne .parse

	; Fall through
.not_found:
	; Panic
	mov rsi, message_no_kernel
	call screen_write
	jmp $

.found:
	; Restore
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret

; Checks and loads the kernel binary.
kernel_load:
	; Store
	push rax
	push rsi

	; Get the address of the kernel binary
	mov rsi, kernel_module
	mov rsi, qword [rsi]
	mov rsi, qword [rsi + hydrogen_info_mod.begin]

	; Check the binary
	call elf64_check
	cmp rax, 0
	je .broken

	; Load the binary
	call elf64_load

	; Get the kernel entry point
	mov rax, qword [rsi + elf64_ehdr.e_entry]
	mov qword [kernel_entry], rax

	; Restore
	pop rsi
	pop rax
	ret

.broken:
	; Panic
	mov rsi, message_kernel_broken
	call screen_write
	jmp $

; Inspects the kernel binary and tries to extract configuration info.
kernel_inspect:
	; Store
	push rdx
	push rsi
	push rdi

	; Get kernel binary
	mov rsi, kernel_module
	mov rsi, qword [rsi]
	mov rsi, qword [rsi + hydrogen_info_mod.begin]

	; Search for string table section
	mov rdi, kernel_inspect_strtbl
	mov rdx, ELF_SHT_STRTAB
	call elf64_section_foreach

	; Found?
	mov rdx, kernel_string_table
	mov rdx, qword [rdx]
	cmp rdx, 0
	je .no_string_table

	; Search for symbol table section
	mov rdi, kernel_inspect_symtbl
	mov rdx, ELF_SHT_SYMTAB
	call elf64_section_foreach

.no_string_table:
	; Restore
	pop rdi
	pop rsi
	pop rdx
	ret

; Sets the location of the kernel binary's string table, given the
; string table's section header.
;
; Parameters:
;	rsi The address of the string table's section header.
;	rdx The address of the kernel binary.
kernel_inspect_strtbl:
	; Store
	push rax

	; Set the string table address
	mov rax, qword [rsi + elf64_shdr.sh_offset]		; Offset into file
	add rax, rdx									; Address
	mov qword [kernel_string_table], rax

	; Restore
	pop rax
	ret

; Parses the kernel binary's symbol table and extracts configuration info.
;
; Parameters:
;	rsi The address of the symbol table's section header.
; 	rdx The address of the kernel binary.
kernel_inspect_symtbl:
	; Store
	push rax
	push rbx
	push rcx
	push rsi
	push rdi

	; Get address of symbols and end address of symbol table
	mov rbx, qword [rsi + elf64_shdr.sh_offset]		; Offset into file
	add rbx, rdx									; Address

	mov rcx, qword [rsi + elf64_shdr.sh_size]		; Size of the section
	add rcx, rbx									; End of symbol table
	mov rsi, rbx									; Address of symbols

	; No symbol?
	cmp rcx, 0
	je .end

	; Iterate through symbols
.symbol:
	; Check the symbol name
	mov edi, dword [rsi + elf64_sym.st_name]
	add rdi, qword [kernel_string_table]

	push rsi
	mov rsi, kernel_symbol_config
	call string_equal
	pop rsi

	; Config symbol?
	cmp rax, 1
	je .found

	; Next?
	add rsi, elf64_sym.end
	cmp rsi, rcx
	jl .symbol

.end:
	; Restore
	pop rdi
	pop rsi
	pop rcx
	pop rbx
	pop rax
	ret

.found:
	; Interpret header
	mov rsi, qword [rsi + elf64_sym.st_value]
	call kernel_inspect_config
	jmp .end

; Parses the kernel binary's hydrogen configuration table.
;
; Parameters:
;	rsi Address of the table in the binary (virtual address!).
;
; Returns:
;	rax 1 if the magic value matched, 0 otherwise.
kernel_inspect_config:
	; Check magic number
	mov eax, dword [rsi + hydrogen_config_table.magic]
	cmp rax,  HYDROGEN_CONFIG_MAGIC
	jne .magic_broken

	; Set config table pointer
	mov qword [config_table], rsi

	; Default IRQ table, if none given
	mov rax, qword [rsi + hydrogen_config_table.irq_table]
	cmp rax, 0
	jne .irq_table_given

	mov rax, config_irq_table_default
	mov qword [rsi + hydrogen_config_table.irq_table], rax
.irq_table_given:

	; Return 1
	mov rax, 1
	ret

.magic_broken:
	; Return 0
	xor rax, rax
	ret

; Uses the config table and maps the stacks to their designated
; virtual addresses.
kernel_map_stacks:
	; Store
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi

	; Get stack_vaddr
	mov rbx, qword [config_table]
	mov rax, qword [rbx + hydrogen_config_table.stack_vaddr]

	; Check if zero
	cmp rax, 0
	je .end

	; Map stacks
	mov rdx, PAGE_FLAG_PW | PAGE_FLAG_GLOBAL
	mov rsi, rax
	mov rdi, stack_heap
	mov rcx, 32

.map_next:
	call page_map

	; Next page
	add rsi, 0x1000
	add rdi, 0x1000
	dec rcx
	cmp rcx, 0
	jne .map_next

	jmp .end

.end:
	; Restore
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret

; Uses the config table and maps the info table to its designated
; virtual address.
kernel_map_info:
	; Store
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi

	; Get info_vaddr
	mov rbx, qword [config_table]
	mov rax, qword [rbx + hydrogen_config_table.info_vaddr]

	; Check if zero
	cmp rax, 0
	je .end

	; Map info table
	mov rdx, PAGE_FLAG_PW | PAGE_FLAG_GLOBAL
	mov rsi, rax
	mov rdi, info_table
	mov rcx, 6

.map_next:
	call page_map

	; Next page
	add rsi, 0x1000
	add rdi, 0x1000
	dec rcx
	cmp rcx, 0
	jne .map_next

	jmp .end

	; Fall through
.end:
	; Restore
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret
