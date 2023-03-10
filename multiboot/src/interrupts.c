#include <kernel.h>
#include <interrupts.h>
#include <taskswitch.h>
#include <kprintf.h>
#include <io.h>

gdt_entry_t gdt_entries[5];
gdt_ptr_t gdt_ptr;
idt_entry_t idt_entries[256];
idt_ptr_t idt_ptr; 
isr_t interrupt_handlers[256];

tss_entry_t tss_entry;

extern void tss_flush();

static void write_tss(s32int num, u16int ss0, s32int esp0)
{
        u32int base = (u32int) &tss_entry;
        u32int limit = base + sizeof(tss_entry_t);

        _memset((char*)&tss_entry, 0, sizeof(tss_entry_t));
        gdt_set_gate(num, base, limit, 0xE9, 0x00);

        tss_entry.ss0  = ss0;   /*kernel stack segment */
        tss_entry.esp0 = esp0;  /*stack pointer */

        tss_entry.cs   = 0x0b;
        tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
}


/* Handlers */

void register_interrupt_handler(u8int n, isr_t handler)
{
	if (interrupt_handlers[n] != 0)
	{
		kprintf("*** BUG *** INT %d claimed twice!\n", n);
		return;
	}
	interrupt_handlers[n] = handler;
} 

/* Default ISR handler */
void isr_handler(registers_t regs)
{
	u8int int_no = regs.int_no & 0xFF;
	if (interrupt_handlers[int_no] != 0)
	{
		isr_t handler = interrupt_handlers[int_no];
		handler(&regs);
		/* After we return these regs will be pushed onto the stack,
		 * in the right order so that when iret returns they become
		 * the process state.
		 */
	}
}

/* Default IRQ handler */
void irq_handler(registers_t regs)
{
	if (regs.int_no >= 40)
		outb(0xA0, 0x20);
	outb(0x20, 0x20);

	isr_handler(regs);
} 

//#####################################################

// IDT Setting Area (Protected Mode)

void idt_set_gate(int num, void *base, short sel, char flags)
{
	idt_entries[num].base_lo = (int)base & 0xFFFF;
	idt_entries[num].base_hi = ((int)base >> 16) & 0xFFFF;
	idt_entries[num].sel = sel;
	idt_entries[num].always0 = 0;
	idt_entries[num].flags = flags | 0x60;
} 

void init_idt()
{
	// Remap the irq table.
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 0x28);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);

	idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
	idt_ptr.base = &idt_entries;
	_memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);
	idt_set_gate( 0, isr0 , 0x08, 0x8E);
	idt_set_gate( 1, isr1 , 0x08, 0x8E);
	idt_set_gate( 2, isr2 , 0x08, 0x8E);
	idt_set_gate( 3, isr3 , 0x08, 0x8E);
	idt_set_gate( 4, isr4 , 0x08, 0x8E);
	idt_set_gate( 5, isr5 , 0x08, 0x8E);
	idt_set_gate( 6, isr6 , 0x08, 0x8E);
	idt_set_gate( 7, isr7 , 0x08, 0x8E);
	idt_set_gate( 8, isr8 , 0x08, 0x8E);
	idt_set_gate( 9, isr9 , 0x08, 0x8E);
	idt_set_gate(10, isr10, 0x08, 0x8E);
	idt_set_gate(11, isr11, 0x08, 0x8E);
	idt_set_gate(12, isr12, 0x08, 0x8E);
	idt_set_gate(13, isr13, 0x08, 0x8E);
	idt_set_gate(14, isr14, 0x08, 0x8E);
	idt_set_gate(15, isr15, 0x08, 0x8E);
	idt_set_gate(16, isr16, 0x08, 0x8E);
	idt_set_gate(17, isr17, 0x08, 0x8E);
	idt_set_gate(18, isr18, 0x08, 0x8E);
	idt_set_gate(19, isr19, 0x08, 0x8E);
	idt_set_gate(20, isr20, 0x08, 0x8E);
	idt_set_gate(21, isr21, 0x08, 0x8E);
	idt_set_gate(22, isr22, 0x08, 0x8E);
	idt_set_gate(23, isr23, 0x08, 0x8E);
	idt_set_gate(24, isr24, 0x08, 0x8E);
	idt_set_gate(25, isr25, 0x08, 0x8E);
	idt_set_gate(26, isr26, 0x08, 0x8E);
	idt_set_gate(27, isr27, 0x08, 0x8E);
	idt_set_gate(28, isr28, 0x08, 0x8E);
	idt_set_gate(29, isr29, 0x08, 0x8E);
	idt_set_gate(30, isr30, 0x08, 0x8E);
	idt_set_gate(31, isr31, 0x08, 0x8E);
	idt_set_gate(32, irq0, 0x08, 0x8E);
	idt_set_gate(33, irq1, 0x08, 0x8E);
	idt_set_gate(34, irq2, 0x08, 0x8E);
	idt_set_gate(35, irq3, 0x08, 0x8E);
	idt_set_gate(36, irq4, 0x08, 0x8E);
	idt_set_gate(37, irq5, 0x08, 0x8E);
	idt_set_gate(38, irq6, 0x08, 0x8E);
	idt_set_gate(39, irq7, 0x08, 0x8E);
	idt_set_gate(40, irq8, 0x08, 0x8E);
	idt_set_gate(41, irq9, 0x08, 0x8E);
	idt_set_gate(42, irq10, 0x08, 0x8E);
	idt_set_gate(43, irq11, 0x08, 0x8E);
	idt_set_gate(44, irq12, 0x08, 0x8E);
	idt_set_gate(45, irq13, 0x08, 0x8E);
	idt_set_gate(46, irq14, 0x08, 0x8E);
	idt_set_gate(47, irq15, 0x08, 0x8E);
	idt_set_gate(48, isr48, 0x08, 0x8E);
	idt_set_gate(49, isr49, 0x08, 0x8E);
	/* Different privilege flags for this, so that it can be called
	 * from usermode (this is the syscall gate)
	 */
	idt_set_gate(50, isr50, 0x08, 0xEE);

	idt_flush(&idt_ptr);
}

// GDT Setting Area (Protected Mode)

void gdt_set_gate(int num, int base, int limit, char access, char gran)
{
	gdt_entries[num].base_low = (base & 0xFFFF);
	gdt_entries[num].base_middle = (base >> 16) & 0xFF;
	gdt_entries[num].base_high = (base >> 24) & 0xFF;
	gdt_entries[num].limit_low = (limit & 0xFFFF);
	gdt_entries[num].granularity = (limit >> 16) & 0x0F;
	gdt_entries[num].granularity |= gran & 0xF0;
	gdt_entries[num].access	= access;
}

void init_gdt()
{
	gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
	gdt_ptr.base  = &gdt_entries;
	gdt_set_gate(0, 0, 0, 0, 0);
	gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment 9a
	gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment 92
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xfA, 0xCF); // User mode code segment fa
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xf2, 0xCF); // User mode data segment f2
	//write_tss(5, 0x10, 0x0);
	//gdt_set_gate(5, 0, 0xFFFFFFFF, 0x9A, 0xCF);
	gdt_flush(&gdt_ptr);
	//tss_flush();
}

void switch_to_user_mode()
{
	// Set up a stack structure for switching to user mode.
	asm volatile("  \
		cli; \
		mov $0x23, %ax; \
		mov %ax, %ds; \
		mov %ax, %es; \
		mov %ax, %fs; \
		mov %ax, %gs; \
                   \
		mov %esp, %eax; \
		pushl $0x1B; \
		pushl %eax; \
		pushf; \ 
		pushl $0x1B; \
		push $1f; \
		iret; \
		1: \
	");
}

