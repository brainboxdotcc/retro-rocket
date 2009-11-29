.code32

#define LOAD_ADDRESS		0x100000 // 1MB
#define PGTABLE_BASE		0x8000
#define MULTIBOOT_PAGE_ALIGN	1 << 0
#define MULTIBOOT_MEMORY_INFO	1 << 1
#define MULTIBOOT_AOUT_KLUDGE	1 << 16
#define MB_MAGIC		0x1BADB002
#define MB_FLAGS		(MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_AOUT_KLUDGE)
#define MB_CHECKSUM		(-(MB_MAGIC + MB_FLAGS))

.align 4

multiboot:
.long	MB_MAGIC		
.long	MB_FLAGS	
.long 	MB_CHECKSUM	
// AOUT Kludge, required for legacy grub to load ELF64
.long	multiboot
.long	LOAD_ADDRESS	
.long	_end
.long	0				
.long	_start	

.global _start
_start:
	mov %ebx, mbinfo
	push %eax

	// Clear screen
	mov     $0x0720, %ax	// White colour attribute, space character
	mov	$0x1000, %ecx
	mov	$0xB8000, %edi
	rep stosw

	pop %eax

	cmp $0x2BADB002, %eax
	jne nomultiboot

	// Check if the CPU has CPUID, if it doesnt its way way too old
	pushf
	pop %eax
	mov %eax, %ebx
	xor $0x200000, %eax
	and $0x200000, %ebx
	push %eax
	popf
	pushf
	pop %eax
	and $0x200000, %eax
	xor %ebx, %eax
	shr $21, %eax
	cmp $0, %eax
	jz not64bit

	// Check the CPU is 64 bit
	mov $0x80000001, %eax
	cpuid
	and $(1 << 29), %edx
	jz not64bit

	// zero out 3 pages of ram where we're going to build some page tables
	xor     %eax, %eax
	mov     $PGTABLE_BASE, %edi
	mov     $0x3000, %ecx
	rep     stosb

	// top level page table
	mov     $((PGTABLE_BASE + 0x1000) | 0x3), %eax
	mov     %eax, PGTABLE_BASE
	mov     $((PGTABLE_BASE + 0x2000) | 0x3), %eax
	mov     %eax, (PGTABLE_BASE + 0x1000)

	// 2MB pages to identity map the first 16MB ram
	mov     $(0 | (1<<7) | 3), %eax
	mov     %eax, (PGTABLE_BASE + 0x2000)
	add     $0x200000, %eax
	mov     %eax, (PGTABLE_BASE + 0x2008)
	add     $0x200000, %eax        
	mov     %eax, (PGTABLE_BASE + 0x2010)
	add     $0x200000, %eax        
	mov     %eax, (PGTABLE_BASE + 0x2018)
	add     $0x200000, %eax        
	mov     %eax, (PGTABLE_BASE + 0x2020)
	add     $0x200000, %eax        
	mov     %eax, (PGTABLE_BASE + 0x2028)
	add     $0x200000, %eax        
	mov     %eax, (PGTABLE_BASE + 0x2030)
	add     $0x200000, %eax        
	mov     %eax, (PGTABLE_BASE + 0x2038)

	// set the page table base
	mov     $PGTABLE_BASE, %eax
	mov     %eax, %cr3

	// load the 64bit gdt
	lgdt	gdt64

	// set CR4.PAE
	mov     %cr4, %eax          
	bts     $5, %eax
	mov     %eax, %cr4        

	// enable long mode      
	// EFER.LME=1            
	mov     $0xc0000080, %ecx
	rdmsr   
	bts     $8, %eax             
	wrmsr  

	// enable paging, which will really enable long mode (CR0.PG)
	mov     %cr0, %eax
	bts     $31, %eax
	mov     %eax, %cr0

	ljmp	$0x08, $(long_64_mode)

need64bit:
.string "A\7 \7s\15i\15x\15t\15y\15 \15f\15o\15u\15r\15 \15b\15i\15t\15 \15p\15r\15o\15c\15e\15s\15s\15o\15r\15 \7i\7s\7 \7r\7e\7q\7u\7i\7r\7e\7d\7 \7t\7o\7 \7r\7u\7n\7 \7t\7h\7i\7s\7 \7o\7p\7e\7r\7a\7t\7i\7n\7g\7 \7s\7y\7s\7t\7e\7m\7.\7"
need64bitend:
needmultiboot:
.string "T\7h\7i\7s\7 \7o\7p\7e\7r\7a\7t\7i\7n\7g\7 \7s\7y\7s\7t\7e\7m\7 \7m\7u\7s\7t\7 \7b\7e\7 \7b\7o\7o\7t\7e\7d\7 \7f\7r\7o\7m\7 \7a\7 \7m\7u\7l\7t\7i\7b\7o\7o\7t\7 \7c\7o\7m\7p\7l\7i\7a\7n\7t\7 \7b\7o\7o\7t\7l\7o\7a\7d\7e\7r\7 \7s\7u\7c\7h\7 \7a\7s\7 \7G\15N\15U\15 \15G\15R\15U\15B\15.\7"
needmultibootend:

// If the multiboot magic is wrong on booting, we just loop
not64bit:
	mov $need64bit, %esi
	mov $(need64bitend - need64bit), %ecx
	jmp print_and_halt

nomultiboot:
	mov $needmultiboot, %esi
	mov $(needmultibootend - needmultiboot), %ecx

print_and_halt:
	shr $1, %ecx
	mov $0xB8000, %edi
	rep movsw
	cli
	hlt


.code64
long_64_mode:

	// set up the stack
	mov		$stack_end, %rsp

	// First parameter: ebx should contain a pointer to the multiboot info structure
	mov		mbinfo, %rdi
	// Second parameter: Stack location
	mov		%rsp, %rsi

	jmp		kmain

// Default exception handler
.global DefaultHandler
DefaultHandler:
	iret

.align 4
gdt64:
	// first entry, also the gdt descriptor
	.word 0xffff
	.long gdt64
	.word 0
	// entry 64bit code segment
    .long 0x00000000
    .long 0x00af9a00

mbinfo:
	.long 0
	.long 0

;.data
;.align 8
stack:
	.skip 0x8000
stack_end:

