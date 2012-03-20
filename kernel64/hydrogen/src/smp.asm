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
; SMP - Initialization
;-------------------------------------------------------------------------------

; Initializes the SMP system by marking the BSP and starting up the APs.
smp_init:
	; Store
	push rax
	push rbx
	push rcx
	push rsi
	push rdi

	; Mark current processor as BSP and ready
	call smp_id					; Get current processor's id
	mov rbx, rax				; Backup id
	info_proc_addr rdi, rax		; Gets the address of the CPU's descriptor
	mov rax, INFO_PROC_FLAG_BSP | INFO_PROC_FLAG_READY | INFO_PROC_FLAG_PRESENT
	add rdi, 2					; Move to flags
	stosw						; Store flags

	; Copy 16 bit init code
	mov rsi, boot16_begin
	mov rcx, boot16_end - boot16_begin
	mov rdi, 0x1000
	call memory_copy

	; Iterate over processors
	xor rcx, rcx
	xor rsi, rsi
	mov cl, byte [info_table.proc_count]
	mov rsi, info_proc

.proc_start:
	; Load and check flags
	xor rax, rax
	mov ax, word [rsi + hydrogen_info_proc.flags]

	; Processor is present?
	mov rbx, rax
	and rbx, INFO_PROC_FLAG_PRESENT
	cmp rbx, 0
	je .proc_next

	; Processor is not ready?
	mov rbx, rax
	and rbx, INFO_PROC_FLAG_READY
	cmp rbx, 0
	jne .proc_next

	; Send init IPI
	xor rax, rax
	mov al, byte [rsi + hydrogen_info_proc.apic_id]
	call lapic_ipi_init

	; Wait for a moment
	push rcx
	mov rcx, 5			; 5 ticks should suffice
	call wait_ticks
	pop rcx

	; Send startup IPI
	call lapic_ipi_startup

	; Wait for processor to become ready
.wait_for_ready:
	xor rax, rax
	mov ax, word [rsi + hydrogen_info_proc.flags]
	and rax, INFO_PROC_FLAG_READY
	cmp rax, 0
	je .wait_for_ready

	; Redirect the PIT to the BSP again, as it is needed
	; for the next wait and must be directed to the BSP
	; in the end anyway.
	call pit_redirect

	; Print message
	push rsi					; Save rsi
	mov rsi, message_ap_started
	call screen_write
	pop rsi

.proc_next:
	; Next processor available?
	add rsi, hydrogen_info_proc.end		; Advance to next processor structure
	dec rcx								; Decrease count of remaining processors
	cmp rcx, 0							; Processor left?
	jne .proc_start

	; Restore
	pop rdi
	pop rsi
	pop rcx
	pop rbx
	pop rax
	ret

smp_init_ap:
	; Store
	push rax
	push rdi

	; Get the current processor's descriptor
	call smp_id
	info_proc_addr rdi, rax

	; Write ready flag
	xor rax, rax
	mov ax, [rdi + hydrogen_info_proc.flags]
	or rax, INFO_PROC_FLAG_READY
	mov word [rdi + hydrogen_info_proc.flags], ax

	; Restore
	pop rdi
	pop rax
	ret

;-------------------------------------------------------------------------------
; SMP - LAPIC
;-------------------------------------------------------------------------------

; Returns the current processor's apic id.
;
; Returns:
; 	rax The current processor's apic id.
smp_id:
	; Store
	push rsi

	; Load id
	xor rax, rax
	mov rsi, qword [info_table.lapic_paddr]
	add rsi, LAPIC_ID_OFFSET
	mov eax, dword [rsi]
	shr rax, 24

	; Restore
	pop rsi
	ret
