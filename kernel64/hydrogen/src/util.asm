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
; Strings
;-------------------------------------------------------------------------------

; Returns the length of the string at rsi.
;
; Parameters:
; 	rsi	The address of the string.
;
; Returns:
;	rax The length of the string.
string_length:
	; Store
	push rcx
	push rsi

	; Increment counter until nul-byte is reached
	xor rcx, rcx					; Counter
	xor rax, rax					; Register for current byte

.next:
	lodsb							; Load byte
	cmp rax, 0						; Null byte?
	je .null

	inc rcx							; Increment counter
	jmp .next						; Next character

.null:
	; Restore
	xchg rax, rcx					; Counter in rax
	pop rsi
	pop rcx
	ret

; Copies a string from rsi to rdi.
;
; Parameters:
;	rsi The address of the string to copy.
;	rdi The address to copy the string to.
;
; Returns:
; 	rdi The address after the copied string's null byte.
string_copy:
	; Store
	push rax
	push rsi

	; Copy bytes until nul-byte is reached
	xor rax, rax					; Register for current byte

.next:
	lodsb							; Load
	stosb							; and write

	cmp rax, 0						; Null byte?
	jne .next

.null:
	; Restore
	pop rsi
	pop rax
	ret

; Checks whether two strings are equal.
;
; Parameters:
;	rsi The address of the first string.
; 	rdi The address of the second string.
;
; Returns:
;	rax 1 if the strings are equal, 0 otherwise.
string_equal:
	; Store
	push rbx
	push rcx
	push rsi
	push rdi

	; Get length for both strings
	call string_length					; First string

	xchg rsi, rdi
	xchg rax, rbx
	call string_length					; Second string

	; Equal length?
	cmp rax, rbx
	jne .not_equal

	; Compare
	mov rcx, rax
	call memory_equal
	jmp .end

.not_equal:
	; Return 0
	xor rax, rax

.end:
	; Restore
	pop rdi
	pop rsi
	pop rcx
	pop rbx
	ret

; Compares two regions of memory and returns whether they are equal.
;
; Parameters:
;	rsi The address of the first region of memory.
;	rdi The address of the second region of memory.
;	rcx The length of the regions.
;
; Returns:
;	rax 1 if they are equal, 0 otherwise.
memory_equal:
	; Store
	push rbx
	push rcx
	push rsi
	push rdi

	; Compare byte-wise
.next_byte:
	; Byte left?
	cmp rcx, 0
	je .equal

	; Compare byte
	xor rax, rax
	xor rbx, rbx
	mov al, byte [rsi]
	mov bl, byte [rdi]
	cmp rax, rbx
	jne .not_equal

	; Next
	dec rcx
	inc rsi
	inc rdi
	jmp .next_byte

.equal:
	; Return 1
	mov rax, 1
	jmp .end

.not_equal:
	; Return 0
	xor rax, rax

.end:
	; Restore
	pop rdi
	pop rsi
	pop rcx
	pop rbx
	ret

; Copies rcx bytes from rsi to rdi.
;
; Parameters:
; 	rsi The address to copy from.
; 	rdi The address to copy to.
; 	rcx The number of bytes to copy.
memory_copy:
	; No bytes to copy?
	cmp rcx, 0
	jne .begin
	ret

.begin:
	; Store
	push rax
	push rcx
	push rsi
	push rdi

.next_byte:
	; Copy byte
	lodsb
	stosb

	; Byte left?
	dec rcx
	cmp rcx, 0
	jne .next_byte

	; Restore
	pop rdi
	pop rsi
	pop rcx
	pop rax
	ret

; Waits a given number of ticks of the PIT.
;
; Parameters:
; 	rcx Number of ticks to wait.
wait_ticks:
	; Store
	push rax
	push rcx

	; Get number of ticks after wait is completed
	add rcx, qword [ticks]

	; Spin on number of ticks
.spin:
	mov rax, qword [ticks]
	cmp rax, rcx
	jl .spin

	; Restore
	pop rcx
	pop rax
	ret

;-------------------------------------------------------------------------------
; Utility - Instruction Sequences
;-------------------------------------------------------------------------------

; Exchanges two values in memory (not thread safe).
;
; Parameters:
;	%1 Address or register with address for the first value.
;	%2 Address of register with address for the second value.
;	%3 Size of the value (byte/word/dword/qword)
;
; Example:
;	xchg_mem 0x1000, rsi, qword
%macro xchg_mem	3
	push rax
	push %3 [%1]
	mov rax, %3 [%2]
	mov %3 [%1], rax
	pop %3 [%2]
	pop rax
%endmacro

; Aligns a register to a 4kB page boundary.
;
; Parameters:
;	%1 Name of the register to align
;
; Example:
; 	align_page rdi
%macro align_page 1
	add %1, 0xFFF			; Add 0x1000-1, for all but for aligned addresses
							; this will advance in the next page
	and %1, ~0xFFF			; Now clear the lower 12 bits
%endmacro

; Macro for retrieving the address of a process descriptor for a processor
; with a given id.
;
; Parameters:
; 	%1 Register to load the address to.
;	%2 APIC id of the processor or register containing the id.
;
; Examples:
; 	info_proc_addr rdi, 0x2
; 	info_proc_addr rsi, rax
%macro info_proc_addr 2
	mov %1, %2				; Load index into target register
	shl %1, 3				; Multiply with four to get offset
	add %1, info_proc		; Add list address
%endmacro

; Macro for printing a two character string to the upper right corner of the
; screen.
;
; Parameters:
; 	%1 The two characters to print.
%macro debug 1
	; Store
	push rax
	push rbx
	push rdi

	mov rdi, HYDROGEN_MEMORY_SCREEN_PADDR + (SCREEN_WIDTH - 2) * 2
	mov rbx, %1
	xor rax, rax

	; First character
	mov al, bl
	or rax, SCREEN_ATTR
	stosw

	; Second character
	mov ax, bx
	shr rax, 8
	or rax, SCREEN_ATTR
	stosw

	; Restore
	pop rdi
	pop rbx
	pop rax
%endmacro
