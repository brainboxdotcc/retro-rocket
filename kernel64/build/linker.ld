OUTPUT_FORMAT(elf64-little)
ENTRY(_start)
SECTIONS
{
    . = 0x00100000;

    .text :
    {
        _code = .;
	*(.multibootheader)
        *(.text)
	*(.rodata*)
        . = ALIGN(4096);
    }

   .data :
   {
        _data = .;
        *(.data)
	*(.debug*)
	*(.bss)
        . = ALIGN(4096);
   }

   .ehframe :
   {
       _ehframe = .;
       *(.ehframe)
        . = ALIGN(4096);
   }

   end = .; _end = .; __end = .;
   codehigh = .;

   /DISCARD/ :
   {
        *(.comment)
   }
}

