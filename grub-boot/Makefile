all:
	nasm -f elf -o loader.o loader.asm
	gcc -o grub-boot.o -c grub-boot.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	ld -T linker.ld -o kernel.bin loader.o grub-boot.o
	perl makepad.pl
	cat  stage1 stage2 pad kernel.bin > floppy.img
