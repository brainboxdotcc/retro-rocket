bits 64
section .text

extern kmain
extern boot_bsp
extern exception_handlers
extern enable_fpu
extern enable_sse

; Entry point for the BSP.
boot_bsp:
	push 0
	call enable_fpu
	call enable_sse
	mov rax, kmain
	call rax
	jmp $

section .data

; make linker silent
section .note.GNU-stack
