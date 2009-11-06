all:
	nasm -f elf -o loader.o asm/loader.asm
	gcc -m32 -o assembly.o -c asm/assembly.s -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o multiboot-kernel.o -c src/multiboot-kernel.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o video.o -c src/video.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o string.o -c src/string.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o printf.o -c src/printf.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o interrupts.o -c src/interrupts.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o errorhandler.o -c src/errorhandler.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o keyboard.o -c src/keyboard.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o timer.o -c src/timer.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o kmalloc.o -c src/kmalloc.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o paging.o -c src/paging.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	ld -m elf_i386 -T linker.ld -o kernel.bin loader.o multiboot-kernel.o video.o printf.o string.o interrupts.o assembly.o errorhandler.o keyboard.o timer.o kmalloc.o paging.o
	chmod ugo-x kernel.bin
	perl makeiso.pl
	-bochs -q -f bochsrc