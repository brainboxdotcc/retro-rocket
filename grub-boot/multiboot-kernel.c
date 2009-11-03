#include "io.h"
#include "video.h"

#define MULTIBOOT_MAGIC 0x2BADB002

void kmain(void* mbd, unsigned int magic)
{
	cursor cursor_position;
	clearscreen(&cursor_position);

	if (magic != MULTIBOOT_MAGIC)
	{
		putstring(&cursor_position,"Invalid magic number from multiboot. System halted.\n");
		for(;;);
	}
	    
	/* You could either use multiboot.h */
	/* (http://www.gnu.org/software/grub/manual/multiboot/multiboot.html#multiboot_002eh) */
	/* or do your offsets yourself. The following is merely an example. */ 
	//char * boot_loader_name =(char*) ((long*)mbd)[16];

	putstring(&cursor_position, "Sixty-Four kernel booting...\n");

	for(;;);
}

