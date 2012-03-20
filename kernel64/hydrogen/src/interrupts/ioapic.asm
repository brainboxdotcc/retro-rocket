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
; Interrupts - I/O APIC - Initialization
;------------------------------------------------------------------------------

; Initializes all I/O APICs.
ioapic_init:
	; Store
	push rcx
	push rsi
	push rdi

	; Check if the system has at least one I/O APIC
	mov cl, byte [info_table.ioapic_count]
	cmp rcx, 0
	je .end

	; Iterate though I/O APICs
	mov rdi, info_ioapic

.handle:
	; Get address of I/O APIC
	mov esi, dword [rdi + hydrogen_info_ioapic.address]

	; Inspect and initialize redirections
	call ioapic_init_inspect
	call ioapic_init_entries

.next:
	; Next?
	dec rcx
	add rdi, hydrogen_info_ioapic.end
	cmp rcx, 0
	jne .handle

.end:
	; Restore
	pop rdi
	pop rsi
	pop rcx
	ret

; Initializes an I/O APIC's redirections.
;
; Parameters:
;	rsi The address of the I/O APIC.
;	rdi The address of the I/O APIC's entry in the info table.
ioapic_init_entries:
	; Store
	push rax
	push rcx

	; Initialize all entries
	mov cl, byte [rdi + hydrogen_info_ioapic.int_count]

.mask:
	dec rcx
	call ioapic_init_entry

.mask_next:
	cmp rcx, 0
	jne .mask

	; Restore
	pop rcx
	pop rax
	ret

; Initializes a given redirection entry.
;
; Standard Fields:
; 	Vector: 0
; 	Delivery Mode: Fixed
; 	Destination Mode: Physical
; 	Pin Polarity: Low Active
; 	Trigger Mode: Edge
; 	Mask: Masked
; 	Destination: Current processor's (BSP) id
;
; Parameters:
; 	rsi The address of the I/O APIC.
; 	rdi The address of the I/O APIC's entry in the info table.
; 	rcx The index of the redirection entry to initialize.
ioapic_init_entry:
	; Store
	push rax
	push rbx

	; Standard fields
	mov rax,	(LAPIC_DELIVERY_FIXED << IOAPIC_REDIR_DELMOD_OFFSET) | \
	         	(LAPIC_MODE_PHYSICAL << IOAPIC_REDIR_DESTMOD_OFFSET) | \
				(IOAPIC_REDIR_INTPOL_LOW << IOAPIC_REDIR_INTPOL_OFFSET) | \
				(LAPIC_TRIGGER_EDGE << IOAPIC_REDIR_TRIGGER_OFFSET) | \
				(1 << IOAPIC_REDIR_MASK_OFFSET)

	; Get BSP APIC id
	xchg rax, rbx
	call smp_id
	shl rax, IOAPIC_REDIR_DEST_OFFSET
	or rax, rbx

	; Write entry
	call ioapic_entry_write

	; Restore
	pop rbx
	pop rax
	ret

; Inspects an I/O further to extract more information than it could be found
; in the ACPI tables.
;
; Extracts the number of interrupts handled by this I/O APIC.
;
; Parameters:
;	rsi The address of the I/O APIC.
;	rdi The address of the I/O APIC's entry in the info table.
ioapic_init_inspect:
	; Store
	push rax
	push rcx

	; Extract max redirection and add one for int count
	mov rcx, IOAPIC_IOAPICVER_INDEX		; Read IOAPICVER register
	call ioapic_reg_read
	shr rax, 16							; Extract bytes 16-23
	and rax, 0xFF
	add rax, 1							; Add one to get interrupt count
	mov byte [rdi + hydrogen_info_ioapic.int_count], al

	; Restore
	pop rcx
	pop rax
	ret

;-------------------------------------------------------------------------------
; Interrupts - I/O APIC - Helper
;------------------------------------------------------------------------------

; Searches for an entry with the given global system interrupt number.
;
; Parameters:
;	rcx The global system interrupt number of the entry.
;
; Returns:
;	rsi The I/O APIC's base address. 0 if not found.
;	rdi The I/O APIC's info table entry address. 0 if not found.
;	rcx The index of the entry. ~0 if not found.
ioapic_entry_get:
	; Store
	push rax
	push rbx

	; Search the entries
	mov bl, byte [info_table.ioapic_count]	; Counter
	mov rdi, info_ioapic

.handle:
	; Get GSI base and interrupt count
	mov eax, dword [rdi + hydrogen_info_ioapic.int_base]
	mov bl, byte [rdi + hydrogen_info_ioapic.int_count]

	; Check if in range
	cmp rcx, rax
	jl .next

	add rbx, rax
	cmp rcx, rbx
	jge .next

	; Handled by this APIC
	sub rcx, rax				; Subtract GSI base to get index
	mov esi, dword [rdi + hydrogen_info_ioapic.address]
	jmp .end

.next:
	; Next?
	add rdi, hydrogen_info_ioapic.end
	dec rbx
	cmp rbx, 0
	jne .handle

.end:
	; Restore
	pop rbx
	pop rax
	ret

	; Writes an entry in an I/O APIC's redirection table.
;
; Parameters:
;	rax The entry to write.
;	ecx The index of the entry.
;	rsi The I/O APIC's base address.
ioapic_entry_write:
	; Store
	push rax
	push rcx

	; Calculate index for lower DWORD
	shl rcx, 1							; * 2: 2 registers for each entry
	add rcx, IOAPIC_IOREDTBL_OFFSET		; Add offset

	; Write lower DWORD
	call ioapic_reg_write

	; Write higher DWORD
	shr rax, 32
	add rcx, 1							; Next register for high DWORD
	call ioapic_reg_write

	; Restore
	pop rcx
	pop rax
	ret

; Reads an entry in an I/O APIC's redirection table.
;
; Parameters:
;	ecx The index of the entry.
;	rsi The I/O APIC's base address.
;
; Returns:
; 	rax The entry value.
ioapic_entry_read:
	; Store
	push rbx
	push rcx

	; Calculate index for lower DWORD
	shl rcx, 1							; *2
	add rcx, IOAPIC_IOREDTBL_OFFSET

	; Read lower DWORD
	call ioapic_reg_read
	mov rbx, rax			; Backup lower DWORD

	; Read higher DWORD
	add rcx, 1
	call ioapic_reg_read

	; Combine
	shr rax, 32
	or rbx, rax
	xchg rbx, rax

	; Restore
	pop rcx
	pop rbx
	ret

; Writes to an (internal) I/O APIC register.
;
; Parameters:
;	eax The value to write.
; 	ecx	The index of the register
;	rsi The I/O APIC's base address.
ioapic_reg_write:
	mov dword [rsi + ioapic.regsel], ecx	; Write register selector
	mov dword [rsi + ioapic.iowin], eax		; Write value
	ret

; Reads an (internal) I/O APIC register.
;
; Parameters:
;	ecx The index of the register.
;	rsi The I/O APIC's base address.
;
; Returns:
;	eax The value of the register.
ioapic_reg_read:
	mov dword [rsi + ioapic.regsel], ecx	; Write register selector
	mov eax, dword [rsi + ioapic.iowin]
	ret
