LDFLAGS=-T build/linker.ld
CFLAGS=-pipe -Wall -nostdlib -nostdinc -ffreestanding -nostartfiles -nodefaultlibs
ASOURCES=loader.o
CSOURCES=kernel.o printf.o string.o video.o memcpy.o apic.o interrupt.o

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
	-bochs -qf bochs.conf

remoterun: all
	-qemu-system-x86_64 -vnc 0.0.0.0:1 -no-reboot -no-shutdown -cdrom sixty-four.iso

harddisk:
	qemu-img create harddisk.img 1G

