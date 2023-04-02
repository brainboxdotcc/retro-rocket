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
	test edx, 1<<25
	jz no_sse
	;SSE is available
	mov rax, cr4
	bts rax, 9
	bts rax, 10
	mov cr4, rax
no_sse:
	ret