#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

// Defined in assembly.s
extern void isr0 ();
extern void isr1 ();
extern void isr2 ();
extern void isr3 ();
extern void isr4 ();
extern void isr5 ();
extern void isr6 ();
extern void isr7 ();
extern void isr8 ();
extern void isr9 ();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void irq0 ();
extern void irq1 ();
extern void irq2 ();
extern void irq3 ();
extern void irq4 ();
extern void irq5 ();
extern void irq6 ();
extern void irq7 ();
extern void irq8 ();
extern void irq9 ();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

// Mappings of IRQ numbers to interrupt numbers

#define IRQ0 32		// System timer
#define IRQ1 33		// Keyboard
#define IRQ2 34		// Cascade
#define IRQ3 35		// COM2
#define IRQ4 36		// COM1
#define IRQ5 37		// Sound
#define IRQ6 38		// FDD
#define IRQ7 39		// Parallel
#define IRQ8 40		// RTC
#define IRQ9 41		// Network, SCSI, unallocated
#define IRQ10 42	// Network, SCSI, unallocated
#define IRQ11 43	// Network, SCSI, unallocated
#define IRQ12 44	// PS2 Mouse
#define IRQ13 45	// FPU
#define IRQ14 46	// Primary IDE
#define IRQ15 47	// Secondary IDE

// An IDT table entry
struct idt_entry_struct
{
   short base_lo;
   short sel;
   char  always0; 
   char  flags;
   short base_hi;
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

// IDT table header, points to a list of idt_entry_struct
struct idt_ptr_struct
{
   short limit;
   void *base; 
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

// A GDT table entry
struct gdt_entry_struct
{
	short limit_low;	   
	short base_low;	   
	char  base_middle;  
	char  access;	  
	char  granularity;
	char  base_high;
} __attribute__((packed));
typedef struct gdt_ptr_struct gdt_ptr_t;

// GDT table header, points to a list of gtdt_entry_struct
struct gdt_ptr_struct
{
	short limit;	
	void *base;	 
} __attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t;

// Flush the IDT with an LIDT instruction
void idt_flush(void * base);

// Flish the GDT with an LGDT instruction
void gdt_flush(void * base);

// Registers, used for obtaining information in an ISR
typedef struct registers
{
	int ds;
	int edi, esi, ebp, esp, ebx, edx, ecx, eax; 
	int int_no, err_code;
	int eip, cs, eflags, useresp, ss;
} registers_t;

// Interrupt handler definition
typedef void (*isr_t)(registers_t);

// Initialise default IDT
void init_idt();

// Register a new interrupt handler
void register_interrupt_handler(int n, isr_t handler);

#endif

