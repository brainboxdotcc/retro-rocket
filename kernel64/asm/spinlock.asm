bits 64
section .text

global init_spinlock
global lock_spinlock
global unlock_spinlock
global get_lapic_address

; Set the byte in the address pointed at by rdi (first integer param) to 0.
init_spinlock:
	mov dword [rdi], 0
	ret

; Lock the spinlock at address pointed to by rdi
lock_spinlock:
	lock bts dword [rdi],0        ;Attempt to acquire the lock (in case lock is uncontended)
	jnc .acquired
 
.retest:
	pause
	test dword [rdi], 1      ;Is the lock free?
	je .retest               ; no, wait
 
	lock bts dword [rdi],0        ;Attempt to acquire the lock
	jc .retest
 
.acquired:
	ret

; Unlock the spinlock at address pointd to by rdi
unlock_spinlock:
	mov dword [rdi], 0
	ret

get_lapic_address:
    push rcx
    push rax
    mov rcx, 1Bh                             ;rcx = model specific Local APIC base MSR
    rdmsr                                   ;edx:eax = model specific Local APIC base MSR (hopefully)
    and rax,0xfffffffffffff000              ;rax = 32-bit physical address of Local APIC (hopefully)
    mov qword [rdi], rax
    pop rax
    pop rcx
    ret
