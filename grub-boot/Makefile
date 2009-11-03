all:
	nasm -f elf -o loader.o loader.asm
	gcc -m32 -o multiboot-kernel.o -c multiboot-kernel.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	ld -m elf_i386 -T linker.ld -o kernel.bin loader.o multiboot-kernel.o
	perl makepad.pl
	bochs -q -f bochsrc
