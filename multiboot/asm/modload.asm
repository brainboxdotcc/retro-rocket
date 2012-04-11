; Module header, identifies functions within a module and defines header magic.

global _start	; start of executable
extern modinit
extern modfini

align 4
ModHeader:
	dd	"MODU"
	dd	modinit
	dd	modfini

_start:
	jmp	modinit
