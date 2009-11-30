#include "../include/kernel.h"
#include "../include/gdt.h"

//extern void DefaultHandler();
//extern void idt_flush();
//extern void gdt_flush();

//extern GDTIDT64* IDT64;
//extern GDTIDT64* GDT64;

GDTEntry64 GDTEntries[256];
Gate64 IDTEntries[256];

void SetGDTEntry(GDTEntry64 *dp, u32 lmode, void *base, u32 size, u32 type, u32 dpl, u32 gran, u32 defopsz)
{
	memset(dp, 0, sizeof(*dp));

	dp->type = type;
	dp->prio = dpl;
	dp->present = 1;
	dp->longmode = 1;	/* 64-bit mode */
	dp->def32 = 0;	/* must be zero for 32-bit operands */
	dp->granularity = gran;	/* 0 = bytes, 1 = pages */
}

void SetIDTEntry(Gate64* dp, void* func, u16 sel, u32 ist, u32 type, u32 dpl)
{
	memset(dp, 0, sizeof (*dp));
    
	dp->looffset = (u32)(u64)func;
	dp->hioffset = (u32)(u64)func >> 16;
	dp->hi64offset = (u64)func >> (16 + 16);
	
	dp->selector = sel;
	dp->ist = ist;
	dp->type = type;
	dp->prio = dpl;
	dp->present = 1;
}

void Interrupt(u64 isrnumber, u64 errorcode)
{
	printf("Interrupt %d\n", isrnumber);
	if (isrnumber != 5)
		wait_forever();
}

void IRQ(u64 isrnumber, u64 errorcode)
{
	printf("IRQ %d\n", isrnumber);
}

void Crash()
{
	printf("erk, crashing!\n");
}

void MakeTables()
{
	int i;
	short CodeSeg = 0xffff, DataSeg = 0xffff;

	printf("Setting limits...\n");
	//GDT64->limit = sizeof(GDTEntry64) * 3 - 1;
	//GDT64->base = (u64)&GDTEntries;

//	IDT64->limit = sizeof(Gate64) * 256 - 1;		/* As above */
//	IDT64->base = (u64)&IDTEntries;			/* As above */

//	printf("IDT limit = %d\n", IDT64->limit);

	asm volatile("mov %%cs, %w0" : "=a"(CodeSeg));
	asm volatile("mov %%ds, %w0" : "=a"(DataSeg));

//	memset(&IDTEntries, 0, sizeof(IDTEntries));
//	memset(&GDTEntries, 0, sizeof(GDTEntries));

 //	 SetGDTEntry(&GDTEntries[GDT_CODE64],
//	 	LONG, /* Long mode code */
//	 	NULL, /* */
 //		0, /* */
 //		30, /* RWX */
 //		0, /* Priv level (ring 0) */
 //		PAGES, /* Bytes or pages? */
 //		OP32);
 //	
 //	SetGDTEntry(&GDTEntries[GDT_DATA64],
//	 	LONG,
 //		NULL,
 //		0,
 //		18, /* RW */
 //		0, /* Ring 0 */
 //		PAGES,
 //		OP32);

//	for (i = 0; i < IDTSZ; i++)
//		SetIDTEntry(&IDTEntries[i], &Crash, CodeSeg, 1, SDT_SYSIGT, 0);

	printf("code64 selector: %08x data64 selector: %08x\n", CodeSeg, DataSeg);

	//gdt_flush();
//	idt_flush();
	printf("...crashing 1...\n");

	asm volatile("int $5");

}

