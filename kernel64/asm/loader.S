//
// Sixty-Four loader trampoline.
//
// This file changes the current operating mode of the processor from protected mode (32-bit) to
// long mode (64-bit), sets up paging and interrupt/exception handling, then passes control to the
// C portions of the kernel. Many of the assembler routines here just marshall low-level events into
// the C code, for example turning interrupts into calls to the Interrupt() function in interrupts.c.
//
// This file also provides multiboot facility for the kernel, by defining a multiboot header with the
// "a.out kludge" required for grub 0.97 to load a 64 bit ELF file.
//

.code32

#define LOAD_ADDRESS		0x100000 // 1MB
#define PGTABLE_BASE		0x8000
#define MULTIBOOT_PAGE_ALIGN	1 << 0
#define MULTIBOOT_MEMORY_INFO	1 << 1
#define MULTIBOOT_AOUT_KLUDGE	1 << 16
#define MB_MAGIC		0x1BADB002
#define MB_FLAGS		(MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_AOUT_KLUDGE)
#define MB_CHECKSUM		(-(MB_MAGIC + MB_FLAGS))

.align 4

multiboot:
.long	MB_MAGIC		
.long	MB_FLAGS	
.long 	MB_CHECKSUM	
// AOUT Kludge, required for legacy grub to load ELF64
.long	multiboot
.long	LOAD_ADDRESS	
.long	_end
.long	0				
.long	_start	

.global _start
_start:
	mov %ebx, mbinfo
	push %eax

	// Clear screen
	mov     $0x0720, %ax	// White colour attribute, space character
	mov	$0x1000, %ecx
	mov	$0xB8000, %edi
	rep stosw

	pop %eax

	cmp $0x2BADB002, %eax
	jne nomultiboot

	// Check if the CPU has CPUID, if it doesnt its way way too old
	pushf
	pop %eax
	mov %eax, %ebx
	xor $0x200000, %eax
	and $0x200000, %ebx
	push %eax
	popf
	pushf
	pop %eax
	and $0x200000, %eax
	xor %ebx, %eax
	shr $21, %eax
	cmp $0, %eax
	jz not64bit

	// Check the CPU is 64 bit
	mov $0x80000001, %eax
	cpuid
	and $(1 << 29), %edx
	jz not64bit

	// Calculate the memory size in megs
	mov mbinfo, %esi
	add $8, %esi
        mov (%esi), %edx
	sub $4, %esi
	add (%esi), %edx
	shr $10, %edx
	inc %edx
	mov %edx, memorysize

	// zero out 3 pages of ram where we're going to build some page tables
	xor     %eax, %eax
	mov     $PGTABLE_BASE, %edi
	mov     $0x3000, %ecx
	rep     stosb

	// top level page table
	mov     $((PGTABLE_BASE + 0x1000) | 0x3), %eax
	mov     %eax, PGTABLE_BASE
	mov     $((PGTABLE_BASE + 0x2000) | 0x3), %eax
	mov     %eax, (PGTABLE_BASE + 0x1000)

	// Identity map the RAM in megabyte pages
	// Up to 0xffffffff. This should include APIC tables.
	mov	memorysize, %edx
	shl	$10, %edx
	mov	$PGTABLE_BASE, %ecx
	add	$0x2000, %ecx
	mov     $(0 | (1<<7) | 3), %eax

create_pagetable:
	mov     %eax, (%ecx)
	add     $0x100000, %eax
	add	$8, %ecx
	cmp	%eax, %edx
	jl	create_pagetable

	// set the page table base
	mov     $PGTABLE_BASE, %eax
	mov     %eax, %cr3

	// load the 64bit gdt
	lgdt	gdt64

	// set CR4.PAE
	mov     %cr4, %eax          
	bts     $5, %eax
	mov     %eax, %cr4        

	// enable long mode      
	// EFER.LME=1            
	mov     $0xc0000080, %ecx
	rdmsr   
	bts     $8, %eax             
	wrmsr  

	// enable paging, which will really enable long mode (CR0.PG)
	mov     %cr0, %eax
	bts     $31, %eax
	mov     %eax, %cr0

	ljmp	$0x08, $(long_64_mode)

need64bit:
.string "A\7 \7s\15i\15x\15t\15y\15 \15f\15o\15u\15r\15 \15b\15i\15t\15 \15p\15r\15o\15c\15e\15s\15s\15o\15r\15 \7i\7s\7 \7r\7e\7q\7u\7i\7r\7e\7d\7 \7t\7o\7 \7r\7u\7n\7 \7t\7h\7i\7s\7 \7o\7p\7e\7r\7a\7t\7i\7n\7g\7 \7s\7y\7s\7t\7e\7m\7.\7"
need64bitend:
needmultiboot:
.string "T\7h\7i\7s\7 \7o\7p\7e\7r\7a\7t\7i\7n\7g\7 \7s\7y\7s\7t\7e\7m\7 \7m\7u\7s\7t\7 \7b\7e\7 \7b\7o\7o\7t\7e\7d\7 \7f\7r\7o\7m\7 \7a\7 \7m\7u\7l\7t\7i\7b\7o\7o\7t\7 \7c\7o\7m\7p\7l\7i\7a\7n\7t\7 \7b\7o\7o\7t\7l\7o\7a\7d\7e\7r\7 \7s\7u\7c\7h\7 \7a\7s\7 \7G\15N\15U\15 \15G\15R\15U\15B\15.\7"
needmultibootend:

// If the multiboot magic is wrong on booting, we just loop
not64bit:
	mov $need64bit, %esi
	mov $(need64bitend - need64bit), %ecx
	jmp print_and_halt

nomultiboot:
	mov $needmultiboot, %esi
	mov $(needmultibootend - needmultiboot), %ecx

print_and_halt:
	shr $1, %ecx
	mov $0xB8000, %edi
	rep movsw
	cli
	hlt


.code64
long_64_mode:

	// set up the stack
	mov		$stack_end, %rsp

	// First parameter: ebx should contain a pointer to the multiboot info structure
	mov		mbinfo, %rdi
	// Second parameter: Stack location
	mov		%rsp, %rsi

	push %rdi
	push %rsi

	// Idea for this taken from baremetal OS, but with some extra stuff
	// to look up the jump addresses from a jump-table.

	xor %rdi, %rdi		// Storage location for IDT gates
	mov $32, %rcx		// There are 32 of them specifically for handing exceptions
	mov $exception_handlers, %r10
exception_gate_loop:
	mov (%r10), %rax		// Get the gate jump-table address in r10
	add $8, %r10
	push %rax			// save the exception gate
	stosw				// Low word of gate address
	mov $0x08, %ax
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

	mov $(51-32), %rcx
interrupt_gate_loop:
	mov (%r10), %rax		// Get the gate jump-table address in r10 again
	add $8, %r10
	push %rax
	stosw
	mov $0x08, %ax
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

	lidt idt64

	// Restore saved params for kmain
	pop %rsi
	pop %rdi
	mov memorysize, %rdx

	jmp kmain

.align 4
gdt64:
	// first entry, also the gdt descriptor
	.word 0xffff
	.long gdt64
	.word 0
	// entry 64bit code segment
	.long 0x0000ffff	// 0xffff: No limit
	.long 0x00af9a00	// 9a: Executable, Read-Only

	// entry for 64bit data segment
	.long 0x0000ffff	// 0xffff: No limit
	.long 0x00af9200	// 92: Non-executable, Read-Write

idt64:
	.word 0xffff
	.quad 0			// Address 0x0 is given over to IDT

mbinfo:
	.quad 0

memorysize:
	.quad 0

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

.data
.align 8
stack:
	.skip 0x8000
stack_end:

