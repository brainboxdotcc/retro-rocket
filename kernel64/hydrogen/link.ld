OUTPUT_FORMAT(elf64-x86-64)
ENTRY(boot32)
SECTIONS
{
    .text 0x100000 :
    {
        code = .;
        *(.multiboot)
        *(.text)
    }

    .data 0x102000 :
    {
        data = .;
        *(.data)
        *(.rodata)
    }
    
    .bss 0x104000 :
    {
    	start = .;
        bss = .;
        *(.bss)
    }

    end = .;
}
