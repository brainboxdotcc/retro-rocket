CFLAGS=-m32 -Wall -nostdlib -nostdinc -ffreestanding -nostartfiles -nodefaultlibs

all:
	nasm -felf -o loader.o asm/loader.asm
	gcc -o assembly.o -c asm/assembly.s ${CFLAGS}
	nasm -felf -o process.o asm/process.asm
	gcc -o multiboot-kernel.o -c src/multiboot-kernel.c ${CFLAGS}
	gcc -o video.o -c src/video.c ${CFLAGS}
	gcc -o string.o -c src/string.c ${CFLAGS}
	gcc -o printf.o -c src/printf.c ${CFLAGS}
	gcc -o interrupts.o -c src/interrupts.c ${CFLAGS}
	gcc -o errorhandler.o -c src/errorhandler.c ${CFLAGS}
	gcc -o keyboard.o -c src/keyboard.c ${CFLAGS}
	gcc -o timer.o -c src/timer.c ${CFLAGS}
	gcc -o kmalloc.o -c src/kmalloc.c ${CFLAGS}
	gcc -o paging.o -c src/paging.c ${CFLAGS}
	gcc -o ordered_array.o -c src/ordered_array.c ${CFLAGS}
	gcc -o ata.o -c src/ata.c ${CFLAGS}
	gcc -o iso9660.o -c src/iso9660.c ${CFLAGS}
	gcc -o debugger.o -c src/debugger.c ${CFLAGS}
	gcc -o memcpy.o -c src/memcpy.c ${CFLAGS}
	gcc -o taskswitch.o -c src/taskswitch.c ${CFLAGS}
	ld -m `perl getsystem.pl` -T linker.ld -o kernel.bin loader.o multiboot-kernel.o video.o printf.o string.o interrupts.o assembly.o errorhandler.o keyboard.o timer.o kmalloc.o paging.o ordered_array.o ata.o iso9660.o debugger.o memcpy.o process.o taskswitch.o
	chmod ugo-x kernel.bin
	perl makeiso.pl

run: all
	#-bochs -q -f bochsrc
	qemu -cdrom sixty-four.iso

