bits 64

section .text

;
global sqrt
sqrt:
	sqrtsd xmm0, xmm0
	ret

; make linker silent
section .note.GNU-stack
