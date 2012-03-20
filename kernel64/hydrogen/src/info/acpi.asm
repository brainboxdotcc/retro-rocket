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
; ACPI - Basic Table Parsing (RSDP/RSDT/XSDT)
;-------------------------------------------------------------------------------

; Parses the ACPI tables.
;
; Searches for the RSDP and passes control to acpi_rsdp_parse.
acpi_parse:
	; Store
	push rax
	push rbx
	push rsi

	; Find RSDP
	mov rsi, 0xE0000			; Start looking here
	mov rbx, 'RSD PTR '			; Look for this
	
.rsdp_next:
	lodsq						; Load potential signature
	
	cmp rax, rbx				; Compare and check if we found it
	je .rsdp_found				; Found (ignore checksum for now)
	
	add rsi, 8					; Else, advance additional 8 bytes, as
								; the signature must be 16 byte aligned
	
	cmp rsi, 0x100000			; Search until we reached this (1MB)
	jl .rsdp_next				; We can search further
	
	jmp .rsdp_not_found			; Else, we do not have ACPI
	
.rsdp_found:
	; Parse RSDP
	sub rsi, 8					; Subtract 8 bytes to get the beginning
								; of the RSDP
	call acpi_rsdp_parse		; Parse the contents of the RSDP
	
	; Restore
	pop rsi
	pop rbx
	pop rax
	ret
	
.rsdp_not_found:
	; TODO: Panic
	jmp $
	
; Parses the RSDP and passes control to either acpi_rsdt_parse or
; acpi_xsdt_parse, depending on the discovered ACPI version.
;
; Parameters:
;	rsi Address of the RSDP.
acpi_rsdp_parse:
	; Store
	push rax
	push rsi

	; Check version
	xor rax, rax				; Clear rax and load version
	mov al, byte [rsi + acpi_rsdp.revision]
	
	cmp rax, 0					; Compare version to 1.0
	je .found_1					; Found 1.0
	
	jmp .found_2				; Found 2.0 or later
	
.found_1:
	xor rax, rax
	mov eax, dword [rsi + acpi_rsdp.rsdt_addr]	; Get rsdt address (to rax)
	xchg rax, rsi								; Write rsdt address to rsi
	call acpi_rsdt_parse						; Parse rsdt
	
	jmp .end					; Jump to end of function
	
.found_2:
	mov rsi, qword [rsi + acpi_rsdp.xsdt_addr]	; Get xsdt address
	call acpi_xsdt_parse						; Parse xsdt
	
	; Fall through to end of function
.end:
	; Restore
	pop rsi
	pop rax
	ret
	
; Macro for both, acpi_rsdp_parse and acpi_xsdt_parse, which are almost
; the same, except for the size of the pointers.
;
; Macro Parameters:
; 	%1 Size of the pointer.
;   %2 Instruction (lodsd or lodsq) for loading the pointer.
%macro acpi_nsdt_parse 2
	; Store
	push rax
	push rcx
	push rsi

	xor rcx, rcx
	mov ecx, dword [rsi + acpi_sdt_header.length] ; Get length
	
	add rsi, 36					; Advance 26 bytes behind header
	sub rcx, 36					; Subtract header size from length
	
	; Parse header pointers
	xor rax, rax				; Clear rax for loading the addresses
	
.next_ptr:
	%2							; Load the table pointer
	
	xchg rax, rsi				; Swap rax and rsi to pass the table
								; pointer in rsi
	call acpi_table_parse		; Generic function for parsing an ACPI
								; table
	xchg rax, rsi				; Sawp rax and rsi again to restore
								; original situation
								
	sub rcx, %1					; Subtract pointer size from remaining
	cmp rcx, 0					; Pointer left?
	jne .next_ptr				; Parse pointer.
	
	; Restore
	pop rsi
	pop rcx
	pop rax
	ret
%endmacro

acpi_rsdt_parse: acpi_nsdt_parse 4, lodsd
acpi_xsdt_parse: acpi_nsdt_parse 8, lodsq

; Parses an ACPI table by passing control to the respective table
; handler.
;
; Parameters:
;	rsi Address of the table to parse.
acpi_table_parse:
	; Store
	push rax

	; Load signature
	xor rax, rax
	mov eax, dword [rsi + acpi_sdt_header.signature]
	
	; Check for MADT
	cmp eax, 'MADT'
	je .madt
	
	cmp eax, 'APIC'
	je .madt
	
	; Unsupported table; return
	jmp .end
	
.madt:
	call acpi_madt_parse
	
	; Fall through to end
.end:
	; Restore
	pop rax
	ret
	
;-------------------------------------------------------------------------------
; ACPI - MADT Parsing
;-------------------------------------------------------------------------------

; Parses the MADT at rsi.
;
; Parameters:
;	rsi The address of the MADT to parse.
acpi_madt_parse:
	; Store
	push rax
	push rcx
	push rdx
	push rsi

	; LAPIC address
	mov eax, dword [rsi + acpi_madt.lapic_addr] 	; Get LAPIC address and
	mov [info_table.lapic_paddr], rax				; write it into the info table

	; PIC availability
	mov rax, [rsi + acpi_madt.flags]		; Get flags
	and rax, ACPI_MADT_PCAT_COMPAT			; Check for PIC availability
	cmp rax, 0
	je .no_pic

	or byte [info_table.flags], INFO_FLAG_PIC ; Set the PIC flag in info table

.no_pic:
	; Devices
	xor rcx, rcx									; Clear rcx for dword length
	mov ecx, dword [rsi + acpi_sdt_header.length]	; Get length of MADT
	add rsi, 44										; Advance behing headers
	sub rcx, 44										; Subtract length of headers

.next_device:
	; Identify device
	mov al, byte [rsi]						; Get device type

	cmp al, ACPI_MADT_TYPE_LAPIC			; LAPIC?
	je .dev_lapic							; Handle LAPIC

	cmp al, ACPI_MADT_TYPE_IOAPIC			; I/O APIC?
	je .dev_ioapic							; Handle I/O APIC

	cmp al, ACPI_MADT_TYPE_ISO				; ISO?
	je .dev_iso								; Handle ISO

	jmp .dev_handled						; Unknown device

.dev_lapic:
	call acpi_madt_lapic_parse				; Parse LAPIC
	jmp .dev_handled						; Device handled

.dev_ioapic:
	call acpi_madt_ioapic_parse				; Parse I/O APIC
	jmp .dev_handled						; Device handled

.dev_iso:
	call acpi_madt_iso_parse				; Parse ISO
	jmp .dev_handled						; Device handled

.dev_handled:
	; Device left?
	xor rdx, rax							; Clear rdx to store length
	mov dl, byte [rsi + 0x1]				; Get length
	sub rcx, rdx							; Subtract from remaining length
	add rsi, rdx							; Add to pointer
	cmp rcx, 0								; Bytes remaining?
	jne .next_device						; Handle next device

	; Restore
	pop rsi
	pop rdx
	pop rcx
	pop rax
	ret

; Parses an LAPIC entry and adds the processor to the info table.
;
; Parameters:
;	rsi The address of the LAPIC entry.
acpi_madt_lapic_parse:
	; Store
	push rax
	push rbx
	push rdx
	push rsi
	push rdi

	; Check if enabled
	mov eax, dword [rsi + acpi_madt_lapic.flags]	; Load flags
	and eax, ACPI_MADT_LAPIC_ENABLED				; Check enabled flag
	cmp eax, 0										; Not enabled?
	je .end											; Ignore this CPU

	; Load ACPI and APIC ids
	xor rbx, rbx
	xor rdx, rdx
	mov bl, byte [rsi + acpi_madt_lapic.acpi_id]	; Load ACPI id
	mov dl, byte [rsi + acpi_madt_lapic.apic_id]	; Load APIC id

	; Add CPU
	info_proc_addr rdi, rdx							; Address for process descr
	mov byte [rdi], bl								; Write ACPI id
	mov byte [rdi + 1], dl							; Write APIC id
	mov word [rdi + 2], INFO_PROC_FLAG_PRESENT		; Write flags

	; Update processor count in info table
	xor rax, rax
	mov al, byte [info_table.proc_count]
	inc dl											; len = max(apic id) + 1
	cmp dl, al										; List expanded?
	jle .end
	mov byte [info_table.proc_count], dl

	; Restore
.end:
	pop rdi
	pop rsi
	pop rdx
	pop rbx
	pop rax
	ret

; Parses an I/O APIC entry and adds it to the info table.
;
; Parameters:
; 	rsi The address of the I/O APIC entry.
acpi_madt_ioapic_parse:
	; Restore
	push rax
	push rbx
	push rcx
	push rdi

	; Load data
	mov al, byte [rsi + acpi_madt_ioapic.ioapic_id]		; Load id
	mov ebx, dword [rsi + acpi_madt_ioapic.ioapic_addr]	; Load address
	mov ecx, dword [rsi + acpi_madt_ioapic.int_base]	; Load interrupt base

	; Write entry to info table
	mov rdi, qword [info_ioapic_next]
	mov byte [rdi + hydrogen_info_ioapic.id], al
	mov dword [rdi + hydrogen_info_ioapic.address], ebx
	mov dword [rdi + hydrogen_info_ioapic.int_base], ecx

	; Next entry in ioapic table
	add qword [info_ioapic_next], hydrogen_info_ioapic.end
	inc byte [info_table.ioapic_count]

	; Restore
	pop rdi
	pop rcx
	pop rbx
	pop rax
	ret

; Parses an interrupt source override entry and adjusts the IRQ to GSI mapping
; in the info table.
;
; Parameters:
; 	rsi The address of the ISO entry.
acpi_madt_iso_parse:
	; Store
	push rax
	push rbx
	push rcx

	; Load data
	mov al, byte [rsi + acpi_madt_iso.source]	; Load IRQ number
	mov ebx, dword [rsi + acpi_madt_iso.gsi]	; Load GSI number
	mov cx, word [rsi + acpi_madt_iso.flags]	; Load flags

	; Store flags in info table
	mov rdi, info_table.irq_flags
	add rdi, rax
	mov byte [rdi], cl

	; Adjust IRQ to GSI mapping in info table
	mov rdi, info_table.irq_to_gsi
	shl rax, 2									; 4 * IRQ: offset in map
	add rdi, rax
	mov dword [rdi], ebx

	; Restore
	pop rcx
	pop rbx
	pop rax
	ret
