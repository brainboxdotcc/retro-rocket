OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_FORMAT(elf64-x86-64)

KERNEL_LMA = 0x0000000001000000; /* 16 MEG */
KERNEL_VMA = 0xffffffff80000000 + KERNEL_LMA;

ENTRY(boot_bsp)

SECTIONS {
	. = KERNEL_VMA;

	_start = .;
	.text : AT(ADDR(.text) - KERNEL_VMA) {
		_code = .;
		*(.text*)
		*(.gnu.linkonce.t*)
		. = ALIGN(0x1000);
	}

	.rodata : AT(ADDR(.rodata) - KERNEL_VMA) {
		_rodata = .;
		*(.rodata*)
		*(.gnu.linkonce.r*)

		. = ALIGN(8);
		__CTOR_LIST__ = .;
		*(.ctors)
		__CTOR_END__ = .;

		. = ALIGN(8);
		__DTOR_LIST__ = .;
		*(.dtors)
		__DTOR_END__ = .;

		. = ALIGN(8);
		__INIT_LIST__ = .;
		KEEP (*(.init_array))
		__INIT_END__ = .;

		. = ALIGN(8);
		__FINI_LIST__ = .;
		KEEP (*(.fini_array))
		__FINI_END__ = .;

		. = ALIGN(8);
		__start_kw_array = .;
		KEEP(*(SORT_BY_NAME(.kw.*)))
		__stop_kw_array = .;

		. = ALIGN(8);
		__dynstr_start = .;
		KEEP(*(.dynstr))
		__dynsym_start = .;
		KEEP(*(.dynsym))
		__dynsym_stop  = .;

		. = ALIGN(8);
		__strtab_start = .;
		KEEP(*(.strtab))
		__symtab_start = .;
		KEEP(*(.symtab))
		__symtab_stop  = .;

		. = ALIGN(0x1000);
	}

	.data : AT(ADDR(.data) - KERNEL_VMA) {
		_data = .;
		*(.data*)
		*(.gnu.linkonce.d*)
		. = ALIGN(0x1000);
	}

	.eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA) {
		_eh_frame = .;
		*(.eh_frame)
		*(.eh_frame_hdr)
		. = ALIGN(0x1000);
	}

	_edata = .;

	.bss : AT(ADDR(.bss) - KERNEL_VMA) {
		_bss = .;
		*(.bss*)
		*(.gnu.linkonce.b*)
		*(COMMON)
		. = ALIGN(0x1000);
	}
	
	_end = .;

	k_end = .;

	/DISCARD/ : {
		*(.comment)
		*(note.*)
	}
}
