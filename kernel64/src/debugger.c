#include <kernel.h>
#include <debugger.h>

void DumpHex(unsigned char* address, u32 length)
{
	int index = 0;
	for(; index < length; index += 16)
	{
		kprintf("%04x: ", index);
		int hex = 0;
		for (; hex < 16; ++hex)
			kprintf("%02x ", address[index + hex]);
		kprintf(" | ");
		for (hex = 0; hex < 16; ++hex)
			put((address[index + hex] < 32 || address[index + hex] > 126) ? '.' : address[index + hex]);

		kprintf("\n");
	}
}

