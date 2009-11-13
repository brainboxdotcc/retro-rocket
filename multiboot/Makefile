LDFLAGS=-m `perl getsystem.pl` -T linker.ld
CFLAGS=-g -m32 -Wall -nostdlib -nostdinc -ffreestanding -nostartfiles -nodefaultlibs
ASOURCES=assembly.o loader.o process.o
CSOURCES=debugger.o interrupts.o keyboard.o multiboot-kernel.o paging.o taskswitch.o video.o ata.o errorhandler.o iso9660.o kmalloc.o memcpy.o ordered_array.o printf.o string.o timer.o

all: source link iso

run: all emulator

clean:
	rm asm/*.o src/*.o

link:
	ld $(LDFLAGS) -o kernel.bin src/*.o asm/*.o
	chmod ugo-x kernel.bin

iso:
	perl makeiso.pl

source:
	make -C asm all
	make -C src all CSOURCES="$(CSOURCES)" CFLAGS="$(CFLAGS)"

emulator:
	qemu -cdrom sixty-four.iso
