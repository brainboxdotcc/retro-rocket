#include "../include/debugger.h"
#include "../include/printf.h"
#include "../include/video.h"

void DumpHex(unsigned char* address, u32int length)
{
	int index = 0;
	for(; index < length; index += 16)
	{
		printf("%04x: ", index);
		int hex = 0;
		for (; hex < 16; ++hex)
			printf("%02x ", address[index + hex]);
		putstring(current_console, " | ");
		for (hex = 0; hex < 16; ++hex)
			put(current_console, (address[index + hex] < 32 || address[index + hex] > 126) ? '.' : address[index + hex]);

		put(current_console, '\n');
	}
}

