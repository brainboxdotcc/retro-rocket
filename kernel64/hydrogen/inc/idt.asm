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

;-------------------------------------------------------------------------------
; Interrupt Descriptor Table
;-------------------------------------------------------------------------------

; Structure of an IDT entry.
;
; .handlerLow		The lower 16 bits of the handler address.
; .cs				The code segment.
; .zero0			Always zero.
; .flags			The flags.
; .handlerMiddle	The next 16 bits of the handler address.
; .handlerHigh		The highest 32 bits of the handler address.
; .zero1:			Always zero.
struc int_idt_entry
	.handlerLow:			RESB 2
	.cs:					RESB 2
	.zero0:					RESB 1
	.flags:					RESB 1
	.handlerMiddle:			RESB 2
	.handlerHigh:			RESB 4
	.zero1:					RESB 4
endstruc

; Layout of the stack on handling an interrupt (without error code!).
;
; .rip			The instruction pointer to return to.
; .cs			The code segment to return to.
; .rflags		The flags to set on return.
; .rsp			The stack pointer to load on return.
; .ss			The stack segment to load on return.
struc int_stack_frame
	.rip:					RESB 8
	.cs:					RESB 8
	.rflags:				RESB 8
	.rsp:					RESB 8
	.ss:					RESB 8
endstruc
