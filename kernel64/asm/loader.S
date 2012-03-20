// These macros are based on JamesM's macros for 32 bit interrupt handlers.
// I have refactored them to use amd64 ABI instead of the stack for parameter passing.

// An ISR with no error code
.macro ISR_NOERR isr
	isr\isr :
		cli
		push %rsi
		push %rdi
		mov $0, %rsi
		mov $ \isr, %rdi
		jmp interrupt_stub
.endm

// An ISR with an error code
.macro ISR_ERR isr
	isr\isr :
		cli
		pop %rax
		push %rsi
		push %rdi
		mov %rax, %rsi
		mov $ \isr, %rdi
		jmp interrupt_stub
.endm

// An IRQ (a special-case ISR, no error code)
.macro IRQ num byte
	irq\num :
		cli
		push %rsi
		push %rdi
		mov $0, %rsi
		mov $ \byte, %rdi
		jmp irq_stub
.endm


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
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
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
ISR_NOERR 50

irq_stub:
	call IRQ
	pop %rdi
	pop %rsi
	sti
	iretq

interrupt_stub:
	call Interrupt
	pop %rdi
	pop %rsi
	sti
	iretq

.globl exception_handlers

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
        .quad isr50


