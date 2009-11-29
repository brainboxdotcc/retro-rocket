global loader						; making entry point visible to linker
global GDTFlush
global IDTFlush
global InterruptHandler

extern Load64BitKernel					; C part of loader
extern EpicFail

; setting up the Multiboot header - see GRUB docs for details
MODULEALIGN equ  1<<0					; align loaded modules on page boundaries
MEMINFO	  equ  1<<1					; provide memory map
FLAGS		 equ	MODULEALIGN | MEMINFO		; this is the Multiboot 'flag' field
MAGIC		 equ	0x1BADB002			; 'magic number' lets bootloader find the header
CHECKSUM	 equ	-(MAGIC + FLAGS)		; checksum required

section .multibootheader
align 4
MultiBootHeader:
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

; reserve initial kernel stack space
STACKSIZE equ 0x4000					; that's 16k.

loader:
	mov esp, stack+STACKSIZE			; set up the stack
	push esp					; pass stack pointer
	push eax					; pass Multiboot magic number
	push ebx					; pass Multiboot info structure

	cli

	call  Load64BitKernel				; call kernel proper
hang:
	hlt						; halt machine should kernel return
	jmp hang

GDTFlush:
	mov eax, [esp+4]				; Get the pointer to the GDT, passed as a parameter.
	lgdt [eax]					; Load the new GDT pointer

	mov ax, 0x10					; 0x10 is the offset in the GDT to our data segment
	mov ds, ax					; Load all data segment selectors
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp 0x08:.flush					; 0x08 is the offset to our code segment
.flush:
	ret

IDTFlush:
	mov eax, [esp+4]
	lidt [eax]					; Load the IDT pointer.
	ret

InterruptHandler:
	cli
	jmp EpicFail

section .bss
align 4
stack:
	resb STACKSIZE					; reserve 16k stack on a doubleword boundary

section .text

