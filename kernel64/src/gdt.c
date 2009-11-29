#include "../include/kernel.h"
#include "../include/gdt.h"

extern void DefaultHandler();

GDTIDT64 GDT64;
GDTIDT64 IDT64;

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

void SetIDTEntry()
{
}

void MakeTables()
{
	int i;

	GDT64.limit = sizeof(GDTEntry64) * 3 - 1;	/* Size of entries, minus 1 */
	GDT64.base = (u64)&GDTEntries;			/* Address of table */

	IDT64.limit = sizeof(Gate64) * 256 - 1;		/* As above */
	GDT64.base = (u64)&IDTEntries;			/* As above */

	memset(&GDTEntries, 0, sizeof(GDTEntries));
	memset(&IDTEntries, 0, sizeof(IDTEntries));

	SetGDTEntry(&GDTEntries[GDT_CODE64],
			LONG,	/* Long mode code */
			NULL,	/* */
			0,	/* */ 
			30,	/* RWX */
			0,	/* Priv level (ring 0) */
			PAGES,	/* Bytes or pages? */	
			OP32);

	SetGDTEntry(&GDTEntries[GDT_DATA64],
			LONG,
			NULL,
			0,
			18,	/* RW */
			0,	/* Ring 0 */
			PAGES,
			OP32);

	for (i = 0; i < IDTSZ; i++)
		SetIDTEntry(&IDTEntries[i], &DefaultHandler, SEL_GDT(GDT_CODE64, 0), 1, SDT_SYSIGT, 0);

}

