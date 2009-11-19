LDFLAGS=-m `perl build/getsystem.pl` -T build/linker.ld
CFLAGS=-g -m32 -Wall -nostdlib -nostdinc -ffreestanding -nostartfiles -nodefaultlibs
ASOURCES=assembly.o loader.o process.o
CSOURCES=debugger.o interrupts.o keyboard.o multiboot-kernel.o paging.o taskswitch.o video.o ata.o errorhandler.o iso9660.o kmalloc.o memcpy.o ordered_array.o printf.o string.o timer.o filesystem.o elf.o devfs.o

all: source link

run: all emulator

clean:
	rm asm/*.o src/*.o

link:
	ld $(LDFLAGS) -o kernel.bin src/*.o asm/*.o
	chmod ugo-x kernel.bin
	perl build/makeiso.pl

source:
	make -C asm all
	make -C src all CSOURCES="$(CSOURCES)" CFLAGS="$(CFLAGS)"

emulator:
	qemu -cdrom sixty-four.iso

debug:
	nohup qemu -s -cdrom sixty-four.iso &
	gdb -command=.gdbargs

remoterun: all
	qemu -vnc 127.0.0.1:0 -cdrom sixty-four.iso

remotedebug: all
	nohup qemu -s -vnc 127.0.0.1:0 -cdrom sixty-four.iso &
	gdb -command=.gdbargs

