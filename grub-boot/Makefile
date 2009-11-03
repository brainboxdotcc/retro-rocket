all:
	nasm -f elf -o loader.o loader.asm
	gcc -o multiboot-kernel.o -c multiboot-kernel.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	ld -T linker.ld -o kernel.bin loader.o multiboot-kernel.o
	perl makepad.pl
	cat  stage1 stage2 pad kernel.bin > floppy.img
