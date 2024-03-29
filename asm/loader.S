.code64
.extern Interrupt
.extern IRQ
.global idt_init
.extern idt64

/*
    0. Null descriptor
    8. 16-bit code descriptor. Base = 0, limit = 0xffff. Readable.
    16. 16-bit data descriptor. Base = 0, limit = 0xffff. Writable.
    24. 32-bit code descriptor. Base = 0, limit = 0xffffffff. Readable.
    32. 32-bit data descriptor. Base = 0, limit = 0xffffffff. Writable.
    40. 64-bit code descriptor. Base and limit irrelevant. Readable.
    48. 64-bit data descriptor. Base and limit irrelevant. Writable.
 */

#define SELECTOR 40

.text

idt_init:
	push %rdi // Storage location for IDT gates
	push %rsi

	// Idea for this taken from baremetal OS, but with some extra stuff
	// to look up the jump addresses from a jump-table.

	mov $32, %rcx		// There are 32 of them specifically for handing exceptions
	movabs $exception_handlers, %r10
exception_gate_loop:
	mov (%r10), %rax		// Get the gate jump-table address in r10
	add $8, %r10
	push %rax			// save the exception gate
	stosw				// Low word of gate address
	mov $SELECTOR, %ax
	stosw				// selector (CS)
	mov $0x8E00, %ax
	stosw 				// exception gate 8E: callable only by ring 0
	pop %rax			// restore gate address
	shr $16, %rax
	stosw				// bits 31-16 of gate address
	shr $16, %rax
	stosl				// bits 63-32 of gate address
	xor %rax, %rax
	stosl				// reserved fields, set to zero
	dec %rcx
	jnz exception_gate_loop

	mov $32, %rcx
interrupt_gate_loop:
	mov (%r10), %rax		// Get the gate jump-table address in r10 again
	add $8, %r10
	push %rax
	stosw
	mov $SELECTOR, %ax
	stosw
	mov $0x8F00, %ax		// Interrupt gate 8F: callable from ring 0 only
	stosw
	pop %rax
	shr $16, %rax
	stosw
	shr $16, %rax
	stosl
	xor %rax, %rax
	stosl
	dec %rcx
	jnz interrupt_gate_loop

	// Restore saved params for kmain
	pop %rsi
	pop %rdi

	retq

// These macros are based on JamesM's macros for 32 bit interrupt handlers.
// I have refactored them to use amd64 ABI instead of the stack for parameter passing.

// An ISR with no error code
.macro ISR_NOERR isr
	isr\isr :
		MPUSHA
		mov $0, %rsi
		movabs $ \isr, %rdi
		jmp interrupt_stub
.endm

// An ISR with an error code
.macro ISR_ERR isr
	isr\isr :
		pop %rax
		MPUSHA
		mov %rax, %rsi
		movabs $ \isr, %rdi
		jmp interrupt_stub
.endm

// An IRQ (a special-case ISR, no error code)
.macro IRQ num byte
	irq\num :
		MPUSHA
		mov $0, %rsi
		mov $ \byte, %rdi
		jmp irq_stub
.endm

.macro MPUSHA
	push %r15
	push %r14
	push %r13
	push %r12
	push %r11
	push %r10
	push %r9
	push %r8
	push %rdi
	push %rsi
	push %rbp
	push %rbx
	push %rdx
	push %rcx
	push %rax
.endm

.macro MPOPA
	pop %rax
	pop %rcx
	pop %rdx
	pop %rbx
	pop %rbp
	pop %rsi
	pop %rdi
	pop %r8
	pop %r9
	pop %r10
	pop %r11
	pop %r12
	pop %r13
	pop %r14
	pop %r15
.endm

isrs:
	ISR_NOERR 0
	ISR_NOERR 1
	ISR_NOERR 2
	ISR_NOERR 3
	ISR_NOERR 4
	ISR_NOERR 5
	ISR_NOERR 6
	ISR_NOERR 7
	ISR_ERR 8
	ISR_NOERR 9
	ISR_ERR 10 
	ISR_ERR 11
	ISR_ERR 12
	ISR_ERR 13
	ISR_ERR 14
	ISR_NOERR 15
	ISR_NOERR 16
	ISR_ERR 17
	ISR_NOERR 18
	ISR_NOERR 19
	ISR_NOERR 20
	ISR_ERR 21
	ISR_NOERR 22
	ISR_NOERR 23
	ISR_NOERR 24
	ISR_NOERR 25
	ISR_NOERR 26
	ISR_NOERR 27
	ISR_NOERR 28
	ISR_ERR 29
	ISR_ERR 30
	ISR_NOERR 31
	IRQ 0, 32
	IRQ 1, 33
	IRQ 2, 34
	IRQ 3, 35
	IRQ 4, 36
	IRQ 5, 37
	IRQ 6, 38
	IRQ 7, 39
	IRQ 8, 40
	IRQ 9, 41
	IRQ 10, 42
	IRQ 11, 43
	IRQ 12, 44
	IRQ 13, 45
	IRQ 14, 46
	IRQ 15, 47
	ISR_NOERR 48
	ISR_NOERR 49
	IRQ 16, 50
	IRQ 17, 51
	IRQ 18, 52
	IRQ 19, 53
	IRQ 20, 54
	IRQ 21, 55
	IRQ 22, 56
	IRQ 23, 57
	IRQ 24, 58
	IRQ 25, 59
	IRQ 26, 60
	IRQ 27, 61
	IRQ 28, 62
	IRQ 29, 63

irq_stub:
	.cfi_startproc
	.cfi_adjust_cfa_offset 8
	.cfi_offset rbp, -120
	call IRQ
	MPOPA
	iretq
	.cfi_endproc

interrupt_stub:
	.cfi_startproc
	.cfi_adjust_cfa_offset 8
	.cfi_offset rbp, -120
	call Interrupt
	MPOPA
	iretq
	.cfi_endproc

exception_handlers:
        .quad isr0
        .quad isr1
        .quad isr2
        .quad isr3
        .quad isr4
        .quad isr5
        .quad isr6
        .quad isr7
        .quad isr8
        .quad isr9
        .quad isr10
        .quad isr11
        .quad isr12
        .quad isr13
        .quad isr14
        .quad isr15
        .quad isr16
        .quad isr17
        .quad isr18
        .quad isr19
        .quad isr20
        .quad isr21
        .quad isr22
        .quad isr23
        .quad isr24
        .quad isr25
        .quad isr26
        .quad isr27
        .quad isr28
        .quad isr29
        .quad isr30
	.quad isr31
        .quad irq0
        .quad irq1
        .quad irq2
        .quad irq3
        .quad irq4
        .quad irq5
        .quad irq6
        .quad irq7
        .quad irq8
        .quad irq9
        .quad irq10
        .quad irq11
        .quad irq12
        .quad irq13
        .quad irq14
        .quad irq15
        .quad isr48
        .quad isr49
        .quad irq16
	.quad irq17
	.quad irq18
	.quad irq19
	.quad irq20
	.quad irq21
	.quad irq22
	.quad irq23
	.quad irq24
	.quad irq25
	.quad irq26
	.quad irq27
	.quad irq28
	.quad irq29

.data


.section .note.GNU-stack
