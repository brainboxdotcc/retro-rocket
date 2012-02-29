global loader           ; making entry point visible to linker
global start		; start of executable
extern kmain            ; kmain is defined elsewhere

start:
 
; setting up the Multiboot header - see GRUB docs for details
MODULEALIGN equ  1<<0                   ; align loaded modules on page boundaries
MEMINFO     equ  1<<1                   ; provide memory map
VIDINFO     equ  1<<2			; provide video info
FLAGS       equ  MODULEALIGN | MEMINFO | VIDINFO  ; this is the Multiboot 'flag' field
MAGIC       equ    0x1BADB002           ; 'magic number' lets bootloader find the header
CHECKSUM    equ -(MAGIC + FLAGS)        ; checksum required
 
section .multibootheader
align 4
MultiBootHeader:
   dd MAGIC
   dd FLAGS
   dd CHECKSUM
 
; reserve initial kernel stack space
; STACKSIZE equ 0x4000                  ; that's 16k.
STACKSIZE equ 0x100000		; 1mb stack
 
loader:
   mov esp, stack+STACKSIZE           ; set up the stack
   push esp                           ; pass stack pointer
   push eax                           ; pass Multiboot magic number
   push ebx                           ; pass Multiboot info structure
 
   call  kmain                       ; call kernel proper
 
   cli
hang:
   hlt                                ; halt machine should kernel return
   jmp   hang
 
section .bss
align 4
stack:
   resb STACKSIZE                     ; reserve 16k stack on a doubleword boundary

section .text

