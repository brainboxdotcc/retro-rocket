/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*			Correct place.  */
/*			Original file taken from Bran's Kernel Development */
/*			tutorials: http://www.osdever.net/bkerndev/index.php. */

ENTRY(loader)
SECTIONS
{
	.multibootheader 0x100000 : {
		*(.multibootheader)
	}
	.text :
	{
		code = .; _code = .; __code = .;
		*(.text)
		. = ALIGN(4096);
	}

	.data :
	{
		data = .; _data = .; __data = .;
		*(.data)
		*(.rodata)
		*(.debug_abbrev)
		*(.debug_info)
		*(.debug_line)
		*(.debug_frame)
		*(.debug_loc)
		*(.debug_pubnames)
		*(.debug_aranges)
		*(.debug_str)
		*(.comment)
		*(.debug_ranges)
		. = ALIGN(4096);
	}

	.bss :
	{
		bss = .; _bss = .; __bss = .;
		*(.bss)
		. = ALIGN(4096);
	}

	end = .; _end = .; __end = .;
}
