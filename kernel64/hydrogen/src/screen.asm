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

section .text
bits 64

; Clears the screen
screen_clear:
	; Store
	push rax
	push rcx
	push rdi

	; Write spaces
	mov ax, ' ' | SCREEN_ATTR				; Space with default attributes
	mov rdi, HYDROGEN_MEMORY_SCREEN_PADDR	; Load address of the video memory
	mov rcx, SCREEN_SIZE					; Load size of the screen
	rep stosw								; Clear screen

	; Reset cursor
	mov word [screen.cursor_x], 0		; Reset cursor x
	mov word [screen.cursor_y], 0		; Reset cursor y
	call screen_update_cursor			; Update hardware cursor

	; Restore
	pop rdi
	pop rcx
	pop rax
	ret

; Writes a null-terminated string to the screen.
;
; Input:
; 	rsi Address of the string to write.
screen_write:
	; Store
	push rax
	push rsi

.next_char:
	; Next character
	xor rax, rax						; Clear rax
	lodsb								; Load character
	cmp al, 0							; Terminate on null-byte
	je .end
	call screen_put						; Put character
	jmp .next_char						; Continue with next character

.end:
	; Restore
	pop rsi
	pop rax
	ret

; Writes the number in rax in hexadecimal representation.
;
; Parameter:
; 	rax The number to write.
screen_write_hex:
	; Store
	push rax
	push rbx
	push rcx

	; Prepare
	mov rcx, 16							; Remaining nibbles
	mov rbx, rax						; Save the number to write in rbx
	xor rax, rax						; Place for character

	; Print prefix (0x)
	mov al, "0"							; Print zero
	call screen_put
	mov al, "x"							; Print x
	call screen_put

.next_nibble:
	; Load nibble
	mov rax, rbx						; Load current value
	shr rax, 60							; Shift value to get nibble
	shl rbx, 4							; Move rbx to next nibble

	; Check nibble
	cmp rax, 10							; Is numerical?
	jl .nibble_digit

	; Get character
	sub rax, 10							; Subtract to get offset
	add rax, "A"						; 10 => "A", 11 => "B" etc.
	jmp .nibble_print					; Print

.nibble_digit:
	; Get digit
	add rax, "0"						; 0 => "0", 1 => "1" etc.

	; Fall through
.nibble_print:
	call screen_put						; Print character

	; Check for next nibble
	dec rcx								; Decrease count of remaining nibbles
	cmp rcx, 0							; No nibble remaining?
	jne .next_nibble

	; Restore
	pop rcx
	pop rbx
	pop rax
	ret


; Puts a single character on the screen.
;
; Parameters:
;	al The character to put.
screen_put:
	; Store
	push rax
	push rdi

	; Check for special chars
	cmp al, 0xD							; Carriage return?
	je .cr

	cmp al, 0xA							; Newline?
	je .nl

	; Any other printable character:
	call screen_cursor_char				; Gets the current position in video mem
	and rax, 0xFF						; Clear everything but the char
	or rax, SCREEN_ATTR					; Add attributes
	stosw								; Write character

	; Update cursor
	inc word [screen.cursor_x]			; x++
	cmp word [screen.cursor_x], 80		; Wrap required?
	jl .end								; If not, jump to end

	; Wrap cursor
	inc word [screen.cursor_y]			; y++
	mov word [screen.cursor_x], 0		; x = 0
	jmp .end							; Jump to end

.cr:
	; Handle carriage return
	mov word [screen.cursor_x], 0		; x = 0
	jmp .end							; Jump to end

.nl:
	; Handle newline
	mov word [screen.cursor_x], 0		; x = 0
	inc word [screen.cursor_y]			; y++
	jmp .end							; Jump to end

.end:
	call screen_scroll					; Scroll (if required)
	call screen_update_cursor			; Update hardware cursor

	; Restore
	pop rdi
	pop rax
	ret

; Returns the offset of the cursor in the screen.
;
; Offset = SCREEN_WIDTH * y + x
;
; Returns:
; 	rax The offset of the cursor in the screen.
screen_offset:
	; Store
	push rdx

	; Calc
	xor rax, rax						; Clear rax to prepare for calc
	mov ax, word [screen.cursor_y]		; Load cursor y
	mov dx, SCREEN_WIDTH				; Load width
	mul dx								; Multiply with width
	add ax, word [screen.cursor_x]		; Add cursor x

	; Restore
	pop rdx
	ret

; Returns the address in video memory of the character the cursor points to.
;
; Address = VideoMemoryAddress + 2 * Offset
;
; Returns:
;	rdi The address of the cursor in video memory.
screen_cursor_char:
	; Store
	push rdx

	; Calc
	xchg rax, rdi						; Swap rax and rdi (return in rdi later)
	call screen_offset					; Get screen offset
	mov rdx, 2							; Load 2
	mul rdx								; Multiply with 2
	add rax, HYDROGEN_MEMORY_SCREEN_PADDR	; Add video memory address
	xchg rax, rdi						; Swap rax and rdi again (rax preserved)

	; Restore
	pop rdx
	ret

; Scrolls the screen by one line, if required.
screen_scroll:
	; Check if scrolling is required
	cmp word [screen.cursor_y], SCREEN_HEIGHT	; Compare y with screen height
	jge .scroll									; Scroll if greater or equal
	ret											; Otherwise no scrolling req

.scroll:
	; Store
	push rax
	push rcx
	push rdi

	; Move current buffer one line up (discarding the top line)
	mov rdi, HYDROGEN_MEMORY_SCREEN_PADDR						; Load the videomem addr
	mov rsi, HYDROGEN_MEMORY_SCREEN_PADDR + (2 * SCREEN_WIDTH)	; Address of 2nd line
	mov rcx, SCREEN_SIZE - SCREEN_WIDTH

.move:
	lodsw								; Load one word and
	stosw								; store it in the upper line
	dec rcx								; One less character remaining
	cmp rcx, 0							; Finished moving?
	jne .move

	; Clear last line by writing spaces to it
	; (rdi should be at the correct location now)
	mov rax, ' ' | SCREEN_ATTR			; Space with default attributes
	mov rcx, SCREEN_WIDTH				; Size of one line
	rep stosw

	; Restore
	pop rdi
	pop rcx
	pop rax
	ret

; Update the screen's hardware cursor position
screen_update_cursor:
	; Store
	push rax
	push rbx
	push rdx

	; Calculate screen offset (-> rax)
	call screen_offset

	; Send low byte
	mov rbx, rax

	mov dx, IO_VIDEO_COMMAND
	mov al, IO_VIDEO_LOW_CUR
	out dx, al

	mov dx, IO_VIDEO_DATA
	mov al, bl
	out dx, al

	; Send high byte
	mov dx, IO_VIDEO_COMMAND
	mov al, IO_VIDEO_HIGH_CUR
	out dx, al

	mov dx, IO_VIDEO_DATA
	mov al, bl
	shr al, 8
	out dx, al

	; Restore
	pop rdx
	pop rbx
	pop rax
	ret
