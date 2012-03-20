OUTPUT_FORMAT(elf64-x86-64)
KERNEL_VMA = 0xFFFFFF0000000000;
ENTRY(boot_bsp)
SECTIONS
{
    . = KERNEL_VMA;

    .text : AT(ADDR(.text) - KERNEL_VMA)
    {
        code = .;
        *(.text)
        *(.rodata*)
        . = ALIGN(4096);
    }

   .data : AT(ADDR(.data) - KERNEL_VMA)
   {
        data = .;
        *(.data)
        . = ALIGN(4096);
   }

   .ehframe : AT(ADDR(.ehframe) - KERNEL_VMA)
   {
       ehframe = .;
       *(.ehframe)
        . = ALIGN(4096);
   }

   .bss : AT(ADDR(.bss) - KERNEL_VMA)
   {
       bss = .;
       *(.bss)
       
        *(COMMON)
       . = ALIGN(4096);
   }

   end = .;

   /DISCARD/ :
   {
        *(.comment)
   }
}
