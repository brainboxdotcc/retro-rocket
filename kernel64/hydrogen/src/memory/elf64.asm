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

; Loads an ELF64 binary into the current address space.
;
; Does not perform integrity checks. See elf64_check.
;
; Parameters:
;	rsi Address of the binary to load.
elf64_load:
	; Store
	push rax
	push rcx
	push rsi
	push r8

	; Get program headers
	xor rcx, rcx
	xor rax, rax
	mov r8, rsi								; Address of the binary
	mov cx, word [rsi + elf64_ehdr.e_phnum]	; Number of program headers
	mov ax, word [rsi + elf64_ehdr.e_phoff]	; Load program header offset
	add rsi, rax							; Add to binary pointer

	; No program header?
	cmp rcx, 0
	je .end

.header:
	; Check if the segment is loadable
	mov eax, dword [rsi + elf64_phdr.p_type]
	cmp rax, ELF_PT_LOAD
	jne .header_next

	; Load segment
	call elf64_load_segment

.header_next:
	; Another header?
	add rsi, elf64_phdr.end
	dec rcx
	cmp rcx, 0
	jne .header

	; Restore
.end:
	pop r8
	pop rsi
	pop rcx
	pop rax
	ret

; Loads a given segment of an ELF64 file.
;
; Parameters:
;	rsi	Address of the program header of the segment to load.
; 	r8	Address of the binary.
elf64_load_segment:
	; Store
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi

	; Infer page flags
	mov rdx, PAGE_FLAG_PRESENT | PAGE_FLAG_GLOBAL

	mov eax, dword [rsi + elf64_phdr.p_flags]	; Check if writable
	and rax, ELF_PF_W
	cmp rax, 0
	je .not_writable
	or rdx, PAGE_FLAG_WRITABLE					; Is writable

.not_writable:
	; Allocate frames and map them
	xor rbx, rbx								; Offset into the segment
	mov rcx, qword [rsi + elf64_phdr.p_memsz]	; Memory size of the segment
	mov rdi, qword [rsi + elf64_phdr.p_vaddr]	; Virtual address to map to

	; No frames?
	cmp rcx, 0
	je .frame_end

.frame_next:
	; Allocate a frame
	call frame_alloc

	; Map the frame
	push rsi
	push rdi
	mov rsi, rdi
	mov rdi, rax
	call page_map

	pop rdi
	pop rsi

	; Next frame?
	add rdi, 0x1000
	add rbx, 0x1000
	cmp rbx, rcx
	jl .frame_next

.frame_end:
	; Copy bytes
	mov rdi, qword [rsi + elf64_phdr.p_vaddr]	; Destination to copy to
	mov rbx, qword [rsi + elf64_phdr.p_offset]	; Offset of source in file
	add rbx, r8 								; Source to copy from
	mov rcx, qword [rsi + elf64_phdr.p_filesz]	; Number of bytes to copy

	; Nothing to copy?
	cmp rcx, 0
	je .copy_end

.copy_next:
	; Copy byte
	xor rax, rax
	mov al, byte [rbx]
	mov byte [rdi], al

	; Next?
	inc rdi
	inc rbx
	dec rcx
	cmp rcx, 0
	jne .copy_next

.copy_end:
	; Clear remaining part of the segment
	mov rcx, qword [rsi + elf64_phdr.p_memsz]	; Full size of segment
	mov rax, qword [rsi + elf64_phdr.p_filesz]	; Size of data part
	mov rdi, qword [rsi + elf64_phdr.p_vaddr]	; Address of segment

	add rdi, rax								; Address of clear part
	sub rcx, rax								; Size of clear part

	xor rax, rax
	rep stosb

	; Restore
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret

; Iterates through the sections of an ELF64 file and calls a callback for
; every section of a given type.
;
; Callback Parameters:
; 	rdx	The address of the binary.
;	rsi The address of the section header.
;
; Parameters:
;	rdx The type of the sections to search.
; 	rsi The address of the ELF64 binary.
;	rdi	The address of the callback.
elf64_section_foreach:
	; Store
	push rax
	push rbx
	push rcx

	; Get section header address and count
	xor rcx, rcx
	mov rbx, qword [rsi + elf64_ehdr.e_shoff]	; Section header offset
	add rbx, rsi								; Section header address
	mov cx, word [rsi + elf64_ehdr.e_shnum]		; Number of section headers

	; No sections?
	cmp rcx, 0
	je .end

.handle:
	; Check section type
	mov eax, dword [rbx + elf64_shdr.sh_type]
	cmp rax, rdx
	jne .next

	; Call callback
	push rdx
	push rsi
	mov rdx, rsi
	mov rsi, rbx
	call rdi
	pop rsi
	pop rdx

.next:
	; Next?
	add rbx, elf64_shdr.end
	dec rcx
	cmp rcx, 0
	jne .handle

.end:
	; Restore
	pop rcx
	pop rbx
	pop rax
	ret

; Checks whether the given binary is a valid, executable, little endian
; ELF64 file that could be used for the kernel.
;
; Parameters:
;	rsi The address of the binary.
;
; Returns:
;	rax 1 if valid, 0 otherwise.
elf64_check:
	; Store
	push rsi

	; Check magic number
	mov eax, dword [rsi + elf64_ehdr.e_ident_mag]
	cmp rax, ELFMAG
	jne .broken

	; Check class
	xor rax, rax
	mov al, byte [rsi + elf64_ehdr.e_ident_class]
	cmp rax, ELFCLASS64
	jne .broken

	; Check data
	xor rax, rax
	mov al, byte [rsi + elf64_ehdr.e_ident_data]
	cmp rax, ELFDATA2LSB
	jne .broken

	; Check type
	xor rax, rax
	mov al, byte [rsi + elf64_ehdr.e_type]
	cmp rax, ELF_ET_EXEC
	jne .broken

	; Valid
	mov rax, 1
	jmp .end

.broken:
	; Broken
	xor rax, rax

.end:
	; Restore
	pop rsi
	ret
