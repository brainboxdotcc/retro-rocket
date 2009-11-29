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
