CC=x86_64-elf-gcc
LD=x86_64-elf-ld
AS=x86_64-elf-as
GAS=x86_64-elf-gas
LDFLAGS=-m elf_x86_64 -T build/linker.ld
CFLAGS=-pipe -Wall -nostdlib -nostdinc -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -I ../include
ASOURCES=boot.o loader.o spinlock.o
CSOURCES=kernel.o printf.o string.o video.o memcpy.o interrupt.o paging.o hydrogen.o debugger.o kmalloc.o timer.o ioapic.o ata.o filesystem.o iso9660.o devfs.o fat32.o errorhandler.o idt.o

all: source link

run: all emulator

clean:
	rm asm/*.o src/*.o

link:
	$(LD) $(LDFLAGS) -o kernel.bin src/*.o asm/*.o
	chmod ugo-x kernel.bin
	perl build/makeiso.pl

source:
	make -C asm all CC="$(CC)" ASOURCES="$(ASOURCES)" CFLAGS="$(CFLAGS)"
	make -C src all CC="$(CC)" CSOURCES="$(CSOURCES)" CFLAGS="$(CFLAGS)"

emulator:
	-qemu-system-x86_64 -soundhw ac97 -usb -usbdevice mouse -m 512 -hda ../harddisk0 -smp 8 -no-reboot -no-shutdown -cdrom sixty-four.iso -boot d

remoterun: all
	-qemu-system-x86_64 -vnc 0.0.0.0:1 -no-reboot -no-shutdown -cdrom sixty-four.iso

harddisk:
	qemu-img create harddisk.img 1G

