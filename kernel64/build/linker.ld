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
        . = ALIGN(4096);
    }

   .data :
   {
        _data = .;
        *(.data)
	*(.rodata*)
	*(.debug*)
        . = ALIGN(4096);
   }

   .ehframe :
   {
       _ehframe = .;
       *(.ehframe)
        . = ALIGN(4096);
   }

   .bss :
   {
       _bss = .;
       *(.bss)
       . = ALIGN(4096);
   }

   _end = .;

   /DISCARD/ :
   {
        *(.comment)
   }
}

