bits 64

section .text

global enable_sse
global enable_fpu


enable_fpu:
	mov rax, cr0
	bts rax, 1
	btr rax, 2
	bts rax, 5
	btr rax, 3
	mov cr0, rax
	fninit
	ret



enable_sse:
	mov eax, 0x1
	cpuid
	test edx, 1 << 25
	jz no_sse
	test edx, 1 << 26
	jz no_sse
	; SSE is available
	mov rax, cr4
	bts rax, 9
	bts rax, 10
	mov cr4, rax
	ret
no_sse:
	mov al, 'O'
	out 0xE9, al
	out 0xE9, al
	mov al, 'F'
	out 0xE9, al
	ret

; make linker silent
section .note.GNU-stack
