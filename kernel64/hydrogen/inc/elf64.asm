; Hydrogen Operating System
; Copyright (C) 2011 Lukas Heidemann
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

; Value for elf64_ehdr.e_ident_mag
%define ELFMAG					0x464C457F	; 0x7F + 'ELF'

; Values for elf64_ehdr.e_indent_class
%define ELFCLASSNONE			0	; Unknown
%define ELFCLASS32				1	; 32 bit arch
%define ELFCLASS64				2	; 64 bit arch

; Values for elf64_ehdr.e_ident_data
%define ELFDATANONE				0	; Unknown
%define ELFDATA2LSB				1	; Little Endian
%define ELFDATA2MSB				2	; Big Endian

; Values for elf64_ehdr.e_type
%define ELF_ET_NONE				0	; Unknown
%define ELF_ET_REL				1	; Relocatable
%define ELF_ET_EXEC				2	; Executable
%define ELF_ET_DYN				3	; Shared Object
%define ELF_ET_CORE				4	; Core File

; Values for elf64_phdr.p_type
%define ELF_PT_NULL				0	; Unused entry
%define ELF_PT_LOAD				1	; Loadable segment
%define ELF_PT_DYNAMIC			2	; Dynamic linking information segment
%define ELF_PT_INTERP			3	; Pathname of interpreter
%define ELF_PT_NOTE				4	; Auxiliary information
%define ELF_PT_SHLIB			5	; Reserved (not used)
%define ELF_PT_PHDR				6	; Location of program header itself

; Values for elf64_phdr.p_flags
%define ELF_PF_X				(1 << 0)	; Executable
%define ELF_PF_W				(1 << 1)	; Writable
%define ELF_PF_R				(1 << 2)	; Readable

; Values for elf64_shdr.sh_type
%define ELF_SHT_NULL			0			; inactive
%define ELF_SHT_PROGBITS		1			; program defined information
%define ELF_SHT_SYMTAB			2			; symbol table section
%define ELF_SHT_STRTAB			3			; string table section
%define ELF_SHT_RELA			4			; relocation section with addends
%define ELF_SHT_HASH			5			; symbol hash table section
%define ELF_SHT_DYNAMIC			6			; dynamic section
%define ELF_SHT_NOTE			7			; note section
%define ELF_SHT_NOBITS			8			; no space section
%define ELF_SHT_REL				9			; relation section without addends
%define ELF_SHT_SHLIB			10			; reserved - purpose unknown
%define ELF_SHT_DYNSYM			11			; dynamic symbol table section
%define ELF_SHT_LOPROC			0x70000000	; reserved range for processor
%define ELF_SHT_HIPROC			0x7FFFFFFF	; specific section header types
%define ELF_SHT_LOUSER			0x80000000	; reserved range for application
%define ELF_SHT_HIUSER			0xFFFFFFFF	; specific indexes

; ELF64 file header.
struc elf64_ehdr
	.e_ident_mag:				RESB 4
	.e_ident_class:				RESB 1
	.e_ident_data:				RESB 1
	.e_ident_version:			RESB 1
	.e_ident_osabi:				RESB 1
	.e_ident_abiversion:		RESB 1
	.e_ident_pad:				RESB 7

	.e_type:					RESB 2
	.e_machine:					RESB 2
	.e_version:					RESB 4
	.e_entry:					RESB 8
	.e_phoff:					RESB 8
	.e_shoff:					RESB 8
	.e_flags:					RESB 4

	.e_ehsize:					RESB 2
	.e_phsize:					RESB 2
	.e_phnum:					RESB 2
	.e_shentsize:				RESB 2
	.e_shnum:					RESB 2
	.e_shstrndx:				RESB 2
	.end:
endstruc

; ELF64 program header.
;
; Contains information on where to load the binary's data and code from and where
; to map it to.
struc elf64_phdr
	.p_type:					RESB 4
	.p_flags:					RESB 4
	.p_offset:					RESB 8
	.p_vaddr:					RESB 8
	.p_paddr:					RESB 8
	.p_filesz:					RESB 8
	.p_memsz:					RESB 8
	.p_align:					RESB 8
	.end:
endstruc

; ELF64 section header.
struc elf64_shdr
	.sh_name:					RESB 4
	.sh_type:					RESB 4
	.sh_flags:					RESB 8
	.sh_addr:					RESB 8
	.sh_offset:					RESB 8
	.sh_size:					RESB 8
	.sh_link:					RESB 4
	.sh_info:					RESB 4
	.sh_addralign:				RESB 8
	.sh_entsize:				RESB 8
	.end:
endstruc

; ELF64 symbol table entry.
struc elf64_sym
	.st_name:					RESB 4
	.st_info:					RESB 1
	.st_other:					RESB 1
	.st_shndx:					RESB 2
	.st_value:					RESB 8
	.st_size:					RESB 8
	.end:
endstruc
