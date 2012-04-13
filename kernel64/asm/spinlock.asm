bits 64
section .text

global init_spinlock
global lock_spinlock
global unlock_spinlock

; Set the byte in the address pointed at by rdi (first integer param) to 0.
init_spinlock:
	lock mov byte [rdi], 0
	ret

; Lock the spinlock at address pointed to by rdi
lock_spinlock:
	lock mov byte [rdi], 1        ;Attempt to acquire the lock (in case lock is uncontended)
	jnc .acquired
.retest:
	pause
	test byte [rdi], 1      ;Is the lock free?
	je .retest               ; no, wait
	lock mov byte [rdi], 1        ;Attempt to acquire the lock
	jc .retest
.acquired:
	ret

; Unlock the spinlock at address pointd to by rdi
unlock_spinlock:
	mov byte [rdi], 0
	ret

