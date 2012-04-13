#include <kernel.h>

/* 64-bit IDT is at 0x0, same position as realmode IDT */
u16 idt64[5] = {0xffff, 0x0000, 0x0000, 0x0000, 0x0000 };

