all:
	nasm -f elf -o loader.o loader.asm
	gcc -m32 -o multiboot-kernel.o -c multiboot-kernel.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o video.o -c video.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o string.o -c string.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	gcc -m32 -o printf.o -c printf.c -Wall -nostdlib -nostartfiles -nodefaultlibs
	ld -m elf_i386 -T linker.ld -o kernel.bin loader.o multiboot-kernel.o video.o printf.o string.o
	chmod ugo-x kernel.bin
	perl makepad.pl
	-bochs -q -f bochsrc
