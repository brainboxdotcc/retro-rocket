#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

#define VIDEO_MEMORY	0xB8000
unsigned char* VideoRam = (unsigned char*)VIDEO_MEMORY;

typedef struct
{
	unsigned short limit_low;
	unsigned short base_low;
	unsigned char  base_middle;
	unsigned char  access;
	unsigned char  granularity;
	unsigned char  base_high;
} __attribute__((packed)) GDTEntry;

typedef struct
{
	unsigned short limit;
	unsigned long base;
} __attribute__((packed)) GDTPointer;

typedef struct
{
	unsigned long flags;
	unsigned long mem_lower;
	unsigned long mem_upper;
	unsigned long boot_device;
	unsigned long commandline;
	unsigned long mods_count;
	unsigned long mods_addr;
	unsigned long elf_headers_num;
	unsigned long elf_headers_size;
	unsigned long elf_headers_addr;
	unsigned long elf_headers_shndx;
	unsigned long mmap_len;
	unsigned long mmap_addr;
} MultiBoot;

typedef struct
{
	unsigned short base_lo;
	unsigned short selector;
	unsigned char reserved0;
	unsigned char flags;
	unsigned short base_hi;
} __attribute__((packed)) IDTEntry;

typedef struct
{
	unsigned short limit;
	unsigned long base;
} __attribute__((packed)) IDTPointer;

extern void GDTFlush(GDTPointer*);
extern void IDTFlush(IDTPointer*);
extern void InterruptHandler();

GDTEntry GDTEntries[4];
GDTPointer GDT;

IDTEntry IDTEntries[256];
IDTPointer IDT;

/* Sets up a GDT gate whos base is always 0, limit is always 0xFFFFFFFF and granularity always 0xCF */
static void SetGate(unsigned int num, unsigned long access)
{
	GDTEntries[num].base_low = GDTEntries[num].base_middle = GDTEntries[num].base_high = 0;
	GDTEntries[num].limit_low	= 0xFFFF;
	GDTEntries[num].granularity	= 0xCF;
	GDTEntries[num].access		= access;
}

static void SetGateIDT(unsigned int num, unsigned long address)
{
	IDTEntries[num].base_lo		= address & 0xFFFF;
	IDTEntries[num].base_hi		= (address >> 16) & 0xFFFF;
	IDTEntries[num].selector	= 0x08;
	IDTEntries[num].reserved0	= 0;
	IDTEntries[num].flags		= 0x8E;
}

/* Outputs an error message on the second line of the text mode screen */
void ShipmentOfFail(const char* error)
{
	int i = 0;
	for (; *error; error++, i++)
		VideoRam[160 + (i * 2)] = *error;
	asm volatile("cli; hlt");
}

void EpicFail()
{
	ShipmentOfFail("Unhandled exception in kernel loader, system halted.");
}

void memset(void *dest, char val, int len)
{
	char *temp = (char *)dest;
		for ( ; len != 0; len--) *temp++ = val;
}


void Load64BitKernel(MultiBoot* mb, unsigned long magicnumber, unsigned long stackaddr)
{
	int i = 0;
	const char* loadingmessage = "Loading sixty-four kernel...";

	/* Clear the screen */
	for (i = 0; i < 0x1000;)
	{
		VideoRam[i++] = ' ';
		VideoRam[i++] = 7;
	}

	for (i = 0; *loadingmessage; loadingmessage++, i++)
		 VideoRam[i * 2] = *loadingmessage;

	//if (magicnumber != MULTIBOOT_HEADER_MAGIC)
	//{
	//	ShipmentOfFail("Invalid multiboot initialisation, system halted.");
	//}
	//else
	{
		/* Create clean 32 bit environment then start long mode initialisation */

		/* Start with a clean GDT and simple paging that identity maps everything */
		GDT.limit = (sizeof(GDTEntries) - 1);
		GDT.base = (unsigned long)&GDTEntries;

		SetGate(0, 0);
		SetGate(1, 0x9A);
		SetGate(2, 0x92);
		GDTFlush(&GDT);

		IDT.limit = (sizeof(IDTEntries) - 1);
		IDT.base = (unsigned long)&IDTEntries;

		memset(&IDTEntries, 0, sizeof(IDTEntries));

		for (i = 0; i < 32; i++)
			SetGateIDT(i, (unsigned long)&InterruptHandler);

		IDTFlush(&IDT);

		ShipmentOfFail("Stopping at end of completed loader.");

	}
}

