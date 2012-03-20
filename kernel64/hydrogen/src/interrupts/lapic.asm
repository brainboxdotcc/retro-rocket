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
; LAPIC - General Functionality
;-------------------------------------------------------------------------------

; Enables the LAPIC.
lapic_enable:
	; Store
	push rax
	push rsi

	; Load the value of the SVR
	mov rsi, [info_table.lapic_paddr]
	add rsi, LAPIC_SVR_OFFSET
	xor rax, rax
	mov eax, dword [rsi]
	or rax, (1 << 8)
	mov dword [rsi], eax

	; Restore
	pop rsi
	pop rax
	ret

; Signals the end of an interrupt for the current processor's LAPIC.
lapic_eoi:
	; Store
	push rax
	push rsi

	; Write zero to EOI
	mov rsi, qword [info_table.lapic_paddr]
	add rsi, LAPIC_EOI_OFFSET
	mov dword [rsi], 0

	; Restore
	pop rsi
	pop rax
	ret

;-------------------------------------------------------------------------------
; LAPIC - Inter-Processor Interrupts
;-------------------------------------------------------------------------------

; Sends an IPI using the current processor's LAPIC.
;
; Parameters:
;	r8  Vector
;	r9  Delivery Mode
;   r10 Destination Mode
; 	r11 Level
;	r12 Trigger Mode
; 	r13 Destination Shorthand
; 	r14 Destination Field
lapic_ipi_send:
	; Store
	push rax
	push rbx
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14

	; Shift and mask parameters
	and r8, 0xFF				; Vector (0..7)
	and r9, 111b				; Delivery Mode (8..10)
	shl r9, 8
	and r10, 1					; Destination Mode (11)
	shl r10, 11
	and r11, 1					; Level (14)
	shl r11, 14
	and r12, 1					; Trigger Mode (15)
	shl r12, 15
	and r13, 11b				; Destination Shorthand (18..19)
	shl r13, 18
	and r14, 0xFF				; Destination Field (56..63)
	shl r14, 56

	; Assemble request
	xor rbx, rbx
	or rbx, r8
	or rbx, r9
	or rbx, r10
	or rbx, r11
	or rbx, r12
	or rbx, r13
	or rbx, r14

	; Write higher dword (write to low fires IPI)
	mov rax, rbx
	shr rax, 32
	mov rdi, qword [info_table.lapic_paddr]
	add rdi, LAPIC_ICR_HIGH_OFFSET
	mov dword [rdi], eax

	; Write lower dword
	xor rax, rax
	mov eax, ebx
	mov rdi, qword [info_table.lapic_paddr]
	add rdi, LAPIC_ICR_LOW_OFFSET
	mov dword [rdi], eax

	; Restore
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rbx
	pop rax
	ret

; Sends an init IPI to the processor with the apic id stored in rax.
;
; Parameters:
;	rax The apic id of the target processor.
lapic_ipi_init:
	; Store
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14

	; Setup IPI
	mov r8, 0x0						; Vector (entry point address >> 12)
	mov r9,	LAPIC_DELIVERY_INIT		; Delivery Mode
	mov r10, LAPIC_MODE_PHYSICAL	; Destination Mode (Physical)
	mov r11, LAPIC_LEVEL_ASSERT		; Level (Assert)
	mov r12, LAPIC_TRIGGER_EDGE		; Trigger Mode (Edge)
	mov r13, LAPIC_SHORT_NONE		; Destination Shorthand (None)
	mov r14, rax					; Destination Field

	; Send IPI
	call lapic_ipi_send

	; Restore
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	ret

; Sends a startup IPI to the processor with the apic id stored in rax.
;
; Parameters:
;	rax The apic id of the target processor.
lapic_ipi_startup:
	; Store
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14

	; Setup IPI
	mov r8, 0x1						; Vector (entry point address >> 12)
	mov r9,	LAPIC_DELIVERY_STARTUP	; Delivery Mode
	mov r10, LAPIC_MODE_PHYSICAL	; Destination Mode (Physical)
	mov r11, LAPIC_LEVEL_ASSERT		; Level (Assert)
	mov r12, LAPIC_TRIGGER_EDGE		; Trigger Mode (Edge)
	mov r13, LAPIC_SHORT_NONE		; Destination Shorthand (None)
	mov r14, rax					; Destination Field

	; Send IPI
	call lapic_ipi_send

	; Restore
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	ret

;-------------------------------------------------------------------------------
; LAPIC - Timer
;-------------------------------------------------------------------------------

; Determines the values for the LAPIC LVT timer register.
lapic_timer_lvt_calc:
	; Store
	push rax
	push rbx
	push rcx
	push rsi

	; Get the timer vector the the timer mode
	xor rcx, rcx
	mov rsi, qword [config_table]
	mov cl, byte [rsi + hydrogen_config_table.lapic_tmrvec]

	mov ebx, dword [rsi + hydrogen_config_table.flags]
	and rbx, HYDROGEN_CONFIG_FLAG_LAPIC_TIMER_ONESHOT
	cmp rbx, 0
	je .mode_periodic

	; Oneshot timer
	mov rbx, (LAPIC_TIMER_MODE_ONESHOT << LAPIC_TIMER_MODE_OFFSET)
	jmp .mode_set

.mode_periodic:
	; Periodic timer
	mov rbx, (LAPIC_TIMER_MODE_PERIODIC << LAPIC_TIMER_MODE_OFFSET)

	; Fallthrough
.mode_set:
	; Set the value for the register
	mov rax, rcx
	or rax, rbx
	mov dword [lapic_timer_lvt], eax

	; Restore
	pop rsi
	pop rcx
	pop rbx
	pop rax
	ret

; Sets the LAPIC's timer's divide configuration register, the LVT timer register
; and the initial count register (in that order).
;
; Parameters:
;	rax The initial count to write.
lapic_timer_set:
	; Store
	push rax
	push rbx
	push rdi

	; Backup initial count
	mov rbx, rax

	; Write divisor 1 to Divide Configuration Register
	mov rdi, qword [info_table.lapic_paddr]
	add rdi, LAPIC_TIMER_DIV_OFFSET

	mov rax, 0xB			; Divisor 1
	stosd

	; Write timer LVT
	mov rdi, qword [info_table.lapic_paddr]
	add rdi, LAPIC_LVT_TIMER_OFFSET

	mov eax, dword [lapic_timer_lvt]
	stosd

	; Write initial count
	mov rdi, qword [info_table.lapic_paddr]
	add rdi, LAPIC_TIMER_INIT_OFFSET

	mov rax, rbx
	stosd

	; Restore
	pop rdi
	pop rbx
	pop rax
	ret

; Calibrates the timer of the current processor's LAPIC.
;
; Requires the PIT IRQ to be directed to the current processor and to be
; unmasked. Also requires interrupts to be enabled.
lapic_timer_calibrate:
	; Store
	push rcx
	push rdx
	push rsi
	push r8

	; Get PIT IRQ vector
	mov rsi, qword [config_table]
	mov rsi, qword [rsi + hydrogen_config_table.irq_table]	; Get IRQ table
	mov cl, byte [rsi + hydrogen_config_irq_entry.vector]	; Get vector (0 offset)

	; Use r8 to determine whether calibration has completed
	xor r8, r8

	; Set new interrupt handler for PIT interrupt
	cli										; Clear interrupts temporarily
	mov rdx, lapic_timer_calibrate_handler
	call int_write_entry
	sti

	; Check whether calibration has completed
.wait:
	cmp r8, 3
	jl .wait

	; Restore interrupt handler
	cli
	mov rdx, pit_tick
	call int_write_entry
	sti

	; Restore
	pop r8
	pop rsi
	pop rdx
	pop rcx
	ret

lapic_timer_calibrate_handler:
	; Store
	push rax
	push rbx
	push rsi

	; Load LAPIC register location
	mov rsi, qword [info_table.lapic_paddr]

	; Check state
	cmp r8, 0		; First run
	je .run_first

	cmp r8, 1		; Second run
	je .run_second

	jmp .end		; Ignore any subsequent run

.run_first:
	; Set initial count register to maximum
	mov rax, 0xFFFFFFFF
	call lapic_timer_set

	; Wait until next PIT interrupt
	jmp .end

.run_second:
	; Get current count value and calculate difference
	mov eax, dword [rsi + LAPIC_TIMER_CUR_OFFSET]
	mov rbx, 0xFFFFFFFF
	sub rbx, rax

	; When n ticks where counted in 1/PIT_FREQ seconds,
	; n * PIT_FREQ ticks would be counted in 1 second.
	mov rax, rbx
	mov rbx, PIT_FREQ
	mul ebx
	mov rbx, rax

	; Write tick count into current processor's info structure
	call smp_id				; Get the current processor's id
	info_proc_addr rsi, rax	; Get the current processor's info structure
	mov dword [rsi + hydrogen_info_proc.lapic_freq], ebx

	; Fallthrough
.end:
	; Increase run count
	inc r8

	; Signal EOI
	call lapic_eoi

	; Restore
	pop rsi
	pop rbx
	pop rax
	iretq
