// loader.S - safer, looped version with automatic gate count

.code64
.extern Interrupt
.extern IRQ
.global idt_init
.extern idt64

#define SELECTOR 40

.text

// Compute number of handlers by label arithmetic
gate_count = (. - exception_handlers) / 8
.global gate_count

idt_init:
    push %rdi
    push %rsi

    movabs $exception_handlers, %r10
    lea idt64(%rip), %r11
    mov 2(%r11), %rdi      // load .base

    mov $94, %rcx

idt_loop:
    mov (%r10), %rax
    add $8, %r10

    push %rax
    stosw
    mov $SELECTOR, %ax
    stosw
    mov $0x8E00, %ax
    stosw
    pop %rax
    shr $16, %rax
    stosw
    shr $16, %rax
    stosl
    xor %eax, %eax
    stosl

    dec %rcx
    jnz idt_loop

    pop %rsi
    pop %rdi
    retq

// Macros (unchanged)
.macro ISR_NOERR isr
isr\isr:
    MPUSHA
    mov $0, %rsi
    movabs $\isr, %rdi
    jmp interrupt_stub
.endm

.macro ISR_ERR isr
isr\isr:
    pop %rax
    MPUSHA
    mov %rax, %rsi
    movabs $\isr, %rdi
    jmp interrupt_stub
.endm

.macro IRQ num byte
irq\num:
    MPUSHA
    mov $0, %rsi
    mov $\byte, %rdi
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
    ISR_ERR   8
    ISR_NOERR 9
    ISR_ERR  10
    ISR_ERR  11
    ISR_ERR  12
    ISR_ERR  13
    ISR_ERR  14
    ISR_NOERR 15
    ISR_NOERR 16
    ISR_ERR  17
    ISR_NOERR 18
    ISR_NOERR 19
    ISR_NOERR 20
    ISR_ERR  21
    ISR_NOERR 22
    ISR_NOERR 23
    ISR_NOERR 24
    ISR_NOERR 25
    ISR_NOERR 26
    ISR_NOERR 27
    ISR_NOERR 28
    ISR_ERR  29
    ISR_ERR  30
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
    call IRQ
    MPOPA
    iretq

interrupt_stub:
    call Interrupt
    MPOPA
    iretq

exception_handlers:
    .quad isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
    .quad isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
    .quad isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    .quad isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    .quad irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
    .quad irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
    .quad isr48, isr49
    .quad irq16, irq17, irq18, irq19, irq20, irq21, irq22, irq23
    .quad irq24, irq25, irq26, irq27, irq28, irq29

.section .note.GNU-stack

